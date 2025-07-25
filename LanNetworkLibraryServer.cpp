#include "LanNetworkLibraryServer.h"
#include <iostream>
#include <winsock2.h>
#include <stdlib.h>
#include <process.h>
#include <windows.h>
#include "Ws2ipdef.h"
#include "ws2tcpip.h"
#include "Profiler.h"

#include "MyRingBuffer.h"
#include <unordered_map>
#include "CPacket.h"
#include "CPacketQueue.h"
#include "ContentsCPacket.h"

#include "LockFreeQueue.h"



#pragma comment(lib,"ws2_32.lib")

struct PacketHeader
{
	unsigned short length;
};


struct MyOverlapped
{
	WSAOVERLAPPED overlapped;
	int Type;
};

struct BufferCount
{
	CPacket* buffers[250];
	long count;
};

#define RELEASEFLAG 0x80000000

struct Session
{
	TLockFreeQueue<CPacket*> send_buffer;

	MyOverlapped send_overlapped;

	MyOverlapped recv_overlapped;
	MyRingBuffer recv_buffer;

	CRITICAL_SECTION enqueue_lock;

	SOCKET sock;
	__int64 session_id;
	bool disconnect_flag;
	bool send_flag;
	long io_count;
	long send_count; // sendTPS용
	BufferCount buffer_count; // 직렬화버퍼 지우기용.


	int* index;//인덱스 저장용

};

unsigned int __stdcall LanNetworkLibraryServer::AcceptThread(LPVOID this_ptr)
{
	LanNetworkLibraryServer* this_for_Accept = (LanNetworkLibraryServer*)this_ptr;
	__int64 i;
	int* temp_index;
	while (1)
	{

		SOCKET client_sock;
		client_sock = accept(this_for_Accept->listen_sock, NULL, NULL);
		if (client_sock == INVALID_SOCKET)
		{
			int error = WSAGetLastError();
			if (error == 10004)
			{
				//리슨소켓 종료로 인한 어셉트 종료.
				break;
			}
			wprintf(L"accept Error %d ", error);
			DebugBreak();

		}


		temp_index = this_for_Accept->FindEmptySessionPTR();
		Session* new_session = &this_for_Accept->session_array[*temp_index];

		new_session->index = temp_index;
		i = *temp_index;


		if (i == this_for_Accept->max_session)
		{
			DebugBreak();
		}

		if (InterlockedIncrement(&this_for_Accept->session_num) > this_for_Accept->max_session)
		{

			DebugBreak();
		}


		new_session->session_id = this_for_Accept->unique_id++;
		new_session->session_id |= (i << 48);
		new_session->buffer_count.count = 0;
		new_session->sock = client_sock;
		new_session->send_flag = FALSE;
		new_session->io_count = 1;
		new_session->recv_overlapped.Type = RECV;
		new_session->send_overlapped.Type = SEND;

		InterlockedIncrement(&this_for_Accept->accept_count);

		CreateIoCompletionPort((HANDLE)client_sock, this_for_Accept->handle_iocp, (ULONG_PTR)&this_for_Accept->session_array[i], 0);

		sockaddr_in clientaddr;
		int addr_len = sizeof(clientaddr);
		getpeername(client_sock, (SOCKADDR*)&clientaddr, &addr_len);
		WCHAR addrl[INET_ADDRSTRLEN];

		if (InetNtopW(AF_INET, &clientaddr.sin_addr, addrl, INET_ADDRSTRLEN) == NULL)
		{

			wprintf(L"InetNtop Error \n");
			DebugBreak();

		}
		this_for_Accept->OnAccept(addrl, ntohs(clientaddr.sin_port), new_session->session_id);

		//wprintf(L"Connect Client : IP  = %s , PORT = %d  , Session ID : %lld\n", addrl, ntohs(clientaddr.sin_port), this_for_Accept->unique_id);
		
		this_for_Accept->ReceiveFirst(new_session);

	}

	return 0;
}


unsigned int __stdcall LanNetworkLibraryServer::WorkerThread(LPVOID this_ptr)
{
	LanNetworkLibraryServer* this_for_worker = (LanNetworkLibraryServer*)this_ptr;
	while (1)
	{
		DWORD cbTransferred;
		MyOverlapped* overlap_ptr;
		Session* target;
		int retval;

		cbTransferred = NULL;
		target = NULL;

		retval = GetQueuedCompletionStatus(this_for_worker->handle_iocp, &cbTransferred, (PULONG_PTR)&target, (LPOVERLAPPED*)&overlap_ptr, INFINITE);

		if (overlap_ptr == NULL && cbTransferred == NULL && target == NULL)
		{
			PostQueuedCompletionStatus(this_for_worker->handle_iocp, NULL, NULL, NULL);
			break;
		}

		if (retval == 0)
		{
			int error = WSAGetLastError();
			if (error == 64)
			{
				if (InterlockedDecrement(&target->io_count) == 0)
				{
					this_for_worker->Release(target);
				}
				continue;
			}
			else
			{
				__debugbreak();
			}

		}


		if (cbTransferred == 0)
		{
			//정상 종료
			wprintf(L"Try Disconnect cbTransferred : 0  Session ID : %lld , overlapped type : %d \n", target->session_id, overlap_ptr->Type);
			if (InterlockedDecrement(&target->io_count) == 0)
			{

				this_for_worker->Release(target);
				continue;
			}
			continue;

		}


		if (overlap_ptr->Type == RECV)
		{
			target->recv_buffer.MoveRear(cbTransferred);	
			if (!this_for_worker->RecvProc(target))
			{
				continue;
			}
			if (!this_for_worker->Receive(target))
			{
				continue;
			}
		}

		if (overlap_ptr->Type == SEND)
		{
			for (int i = 0; i < target->buffer_count.count; ++i)
			{
				if (target->buffer_count.buffers[i]->DecreaseRefCount() == 0)
				{
					CPacket::Free(target->buffer_count.buffers[i]);		

				}
				
			}
			target->buffer_count.count = 0;

			InterlockedExchange8((char*)&target->send_flag, 0);
			
			if (!this_for_worker->SendPost(target))
			{
				continue;
			}

		}

		if (InterlockedDecrement(&target->io_count) == 0)
		{
			this_for_worker->Release(target);
			continue;
		}


	}
	return 0;
}

unsigned int __stdcall LanNetworkLibraryServer::MonitorThread(LPVOID this_ptr)
{
	LanNetworkLibraryServer* this_for_monitor = (LanNetworkLibraryServer*)this_ptr;

	while (1)
	{

		this_for_monitor->accept_TPS = this_for_monitor->accept_count;
		this_for_monitor->recv_message_TPS = this_for_monitor->recv_message_count;
		this_for_monitor->send_message_TPS = this_for_monitor->send_message_count;

		InterlockedExchange(&this_for_monitor->accept_count, 0);
		InterlockedExchange(&this_for_monitor->recv_message_count, 0);
		InterlockedExchange(&this_for_monitor->send_message_count, 0);

		Sleep(1000);

	}




	return 0;
}






LanNetworkLibraryServer::LanNetworkLibraryServer()
	: accept_TPS(0), recv_message_TPS(0), send_message_TPS(0), accept_count(0),
	recv_message_count(0), send_message_count(0), max_session(0), session_num(0), threads_num(0), unique_id(0),
	index_list(0, false)
	
{
	for (int i = 0; i < 200; i++)
	{
		buf_count_total[i][0] = 0;
		buf_count_total[i][1] = 0;
	}

}

LanNetworkLibraryServer::~LanNetworkLibraryServer()
{
}


bool LanNetworkLibraryServer::Start(const char* server_IP, unsigned int  server_port, unsigned int worker_num, unsigned int concurrent_threads, unsigned int nagle, unsigned int sessions, unsigned int header)
{



	max_session = sessions;
	session_num = 0;
	session_array = new Session[max_session];
	header_size = header;
	int** temp = new int* [max_session];



	for (unsigned int i = 0; i < max_session; ++i)
	{
		InitializeCriticalSection(&session_array[i].enqueue_lock);
		session_array[i].send_count = 0;
		temp[i] = index_list.Alloc();
		*temp[i] = i;
	}
	for (unsigned int i = 0; i < max_session; ++i)
	{
		index_list.Free(temp[i]);

	}

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		DebugBreak();
	}
	//아마 4개 세팅? 
	handle_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, concurrent_threads);
	if (handle_iocp == NULL)
	{
		DebugBreak();
	}


	listen_sock = socket(AF_INET, SOCK_STREAM, 0);

	if (listen_sock == INVALID_SOCKET)
	{
		int Error = WSAGetLastError();
		wprintf(L"ListenSocket Error %d \n", Error);

		DebugBreak();
	}


	SOCKADDR_IN server_address;
	ZeroMemory(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	InetPtonA(AF_INET, server_IP, &server_address.sin_addr);
	//server_address.sin_addr.s_addr = htonl(INADDR_ANY); // 문자열 파싱 해야 함 .
	server_address.sin_port = htons(server_port);

	int bind_return = bind(listen_sock, (const sockaddr*)&server_address, sizeof(server_address));
	if (bind_return == SOCKET_ERROR)
	{
		bind_return = WSAGetLastError();

		wprintf(L"BindReturn Error : %d \n", bind_return);

		DebugBreak();
	}
	DWORD OptionVal = 0;

	int socket_option_return = setsockopt(listen_sock, SOL_SOCKET, SO_SNDBUF, (const char*)&OptionVal, sizeof(OptionVal));
	if (socket_option_return == SOCKET_ERROR)
	{
		socket_option_return = WSAGetLastError();

		wprintf(L"SocketOptionReturn Error : %d \n", socket_option_return);
		DebugBreak();
	}

	

	LINGER linger;
	linger.l_linger = 0;
	linger.l_onoff = 1;

	int SocketOption = setsockopt(listen_sock, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(linger));
	if (SocketOption == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		printf("setsockopt Error %d ", error);

		DebugBreak();
	}

	if (nagle)
	{
		DWORD NoDelay = 1;

		int NoDelayOption = setsockopt(listen_sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&NoDelay, sizeof(NoDelay));
		if (NoDelayOption == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			printf("setsockopt Error %d ", error);

			DebugBreak();
		}
	}

	

	int listen_return = listen(listen_sock, SOMAXCONN_HINT(7000));
	//int listen_return = listen(listen_sock, SOMAXCONN);
	if (listen_return == SOCKET_ERROR)
	{
		listen_return = WSAGetLastError();

		wprintf(L"Listen Error : %d \n", listen_return);
		DebugBreak();
	}

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	threads_num = worker_num;

	threads = new HANDLE[threads_num];

	for (unsigned int i = 0; i < threads_num; ++i)
	{
		threads[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, this, 0, NULL);

		if (threads[i] == NULL)
		{
			wprintf(L"_beginthreadex Failed \n");
			DebugBreak();
		}

	}

	accept_thread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, NULL);

	if (accept_thread == NULL)
	{
		wprintf(L"_beginthreadex Failed \n");
		DebugBreak();
	}

	monitor_thread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, NULL);

	if (monitor_thread == NULL)
	{
		wprintf(L"_beginthreadex Failed \n");
		DebugBreak();
	}


	return true;
}

bool LanNetworkLibraryServer::Stop()
{

	PostQueuedCompletionStatus(handle_iocp, NULL, (ULONG_PTR)NULL, NULL);

	WaitForMultipleObjects(threads_num, threads, TRUE, INFINITE);

	closesocket(listen_sock);

	WaitForSingleObject(accept_thread, INFINITE);


	return true;
}

int LanNetworkLibraryServer::GetSessionCount()
{
	return session_num;
}

bool LanNetworkLibraryServer::Disconnect(__int64 session_ID)
{


	Session* target;
	unsigned int i = FindSession(session_ID);
	target = &session_array[i];


	int local_count = InterlockedIncrement(&target->io_count);

	if ((local_count & RELEASEFLAG) == RELEASEFLAG)
	{
		if (InterlockedDecrement(&target->io_count) == 0)
		{
			Release(target);
		}
		return false;
	}

	//이러면 해제 이후 다시 재사용 된 상태. 다시 카운트 감소시키고 리턴시키기. 
	if (target->session_id != session_ID)
	{
		//근데 생각해보니 재사용되었고 또 삭제되어야 했을 수도 있음. 
		//목적과 다른 녀석이지만 얘도 내가 증가시켰으므로 감소 후 지워주기. 
		if (InterlockedDecrement(&target->io_count) == 0)
		{
			Release(target);
		}
		return false;
	}

	// Disconnect 1회 제한
	if (InterlockedExchange8((char*)&target->disconnect_flag, 1) == 1)
	{
		if (InterlockedDecrement(&target->io_count) == 0)
		{
			Release(target);
			
		}
		return false;
	}

	CancelIoEx((HANDLE)target->sock,nullptr);

	if (InterlockedDecrement(&target->io_count) == 0)
	{
		Release(target);
		return false;
	}


	return false;
}

bool LanNetworkLibraryServer::SendPacket(__int64 session_ID, ContentsCPacket send_packet)
{

	Session* target;
	unsigned int i = FindSession(session_ID);

	target = &session_array[i];
	int local_count = InterlockedIncrement(&target->io_count);

	if ((local_count & RELEASEFLAG) == RELEASEFLAG)
	{
		if (InterlockedDecrement(&target->io_count) == 0)
		{
			Release(target);
		}
		return false;
	}

	//이러면 해제 이후 다시 재사용 된 상태. 다시 카운트 감소시키고 리턴시키기. 
	if (target->session_id != session_ID)
	{
		//근데 생각해보니 재사용되었고 또 삭제되어야 했을 수도 있음. 
		//목적과 다른 녀석이지만 얘도 내가 증가시켰으므로 감소 후 지워주기. 
		if (InterlockedDecrement(&target->io_count) == 0)
		{
			Release(target);
		}
		return false;
	}


	


	//인큐 , Send 
	AddHeader(send_packet.packet_buffer);

	int enqueue_return = target->send_buffer.Enqueue(send_packet.packet_buffer);
	send_packet.packet_buffer->IncreaseRefCount();
	InterlockedIncrement(&target->send_count);
	if (enqueue_return == false)
	{
		wprintf(L"EnqueueFail in SendPacketUnicast Session Id : %lld\n", target->session_id);
		DebugBreak();
		//Disconnect(target->session_id); //리턴이 2든 아니든 일단 호출
		return false;
	}

	if (!SendPost(target))
	{
		return false;
	}



	if (InterlockedDecrement(&target->io_count) == 0)
	{
		Release(target);
		return false;
	}


	return false;
}

bool LanNetworkLibraryServer::SendPost(Session* target)
{

	if (target->disconnect_flag == 1)
	{
		return false;
	}

	int WSASend_return;

	if (InterlockedExchange8((char*)&target->send_flag, 1) == 0)
	{

		WSABUF local_wsabuf[250];

		int buf_count = 0;


		while (1)
		{
			CPacket* temp = nullptr;

			if (target->send_buffer.Dequeue(&temp) == false)
			{
				break;
			}
			target->buffer_count.buffers[buf_count] = temp;
			local_wsabuf[buf_count].buf = temp->GetBufferPtr() + LIBHEADERSIZE - header_size;
			local_wsabuf[buf_count].len = temp->GetDataSize() + header_size;
			buf_count++;

		}
		target->buffer_count.count = buf_count;

		if (buf_count == 0)
		{
			InterlockedExchange8((char*)&target->send_flag, 0);
			if (target->send_buffer.GetSize() != 0)
			{
				SendPost(target);
			}
			return true;
		}


		

		InterlockedAdd(&send_message_count, target->send_count);
		InterlockedExchange(&target->send_count, 0);
		

		DWORD sendbytes = 0;
		InterlockedIncrement(&target->io_count);


		ZeroMemory(&target->send_overlapped.overlapped, sizeof(target->send_overlapped.overlapped));

		WSASend_return = WSASend(target->sock, local_wsabuf, buf_count, &sendbytes, 0, &target->send_overlapped.overlapped, NULL);
		if (WSASend_return == SOCKET_ERROR)
		{
			int WSASendError = WSAGetLastError();

			if (WSASendError == WSA_IO_PENDING)
			{

				if (target->disconnect_flag == 1)
				{
					CancelIoEx((HANDLE)target->sock, nullptr);
					
				}

			}

			if (WSASendError != WSA_IO_PENDING)
			{

				if (InterlockedDecrement(&target->io_count) == 0)
				{
					Release(target);
					return false;
				}

			}

		}

	}

	return true;
}



bool LanNetworkLibraryServer::ReceiveFirst(Session* new_session)
{
	WSABUF wsabuf;
	wsabuf.buf = new_session->recv_buffer.GetRearBufferPtr();
	wsabuf.len = new_session->recv_buffer.GetFreeSize();

	DWORD recvbytes;
	DWORD flags = 0;
	int retval;

	ZeroMemory(&new_session->recv_overlapped.overlapped, sizeof(new_session->recv_overlapped.overlapped));
	retval = WSARecv(new_session->sock, &wsabuf, 1, &recvbytes, &flags, (WSAOVERLAPPED*)&new_session->recv_overlapped.overlapped, 0);

	if (retval == SOCKET_ERROR)
	{
		int WSARecv_error = WSAGetLastError();
		if (WSARecv_error != WSA_IO_PENDING)
		{

			wprintf(L"In First WSARecvError : %d  , Session ID : %lld\n", WSARecv_error, new_session->session_id);

			if (InterlockedDecrement(&new_session->io_count) == 0)
			{
				Release(new_session);
				return false;
			}

			return true;

		}

	}


	return true;
}

bool LanNetworkLibraryServer::RecvProc(Session* target)
{
	while (1)
	{
		int target_recv_buffer_size = target->recv_buffer.GetUseSize();
		if (target_recv_buffer_size == 0)
		{
			break;
		}

		PacketHeader header;
		if (target_recv_buffer_size < sizeof(header))
		{
			break;
		}

		if (target->recv_buffer.Peek((char*)&header, sizeof(header)) != sizeof(header))
		{
			//__debugbreak();
			break;
		}

		if (target_recv_buffer_size < sizeof(header) + header.length)
		{
			break;
		}

		unsigned int receive_dequeue_header_size = target->recv_buffer.MoveFront(sizeof(header));

		//wprintf(L"## ID : %d  DequeueHeaderSize : %d  ", Target->SessionID, ReceiveQDequeueHeaderSize);

		CPacket* packet_buffer = CPacket::Alloc();

		packet_buffer->IncreaseRefCount();

		unsigned int receive_dequeue_packet_size = target->recv_buffer.Dequeue(packet_buffer->GetBufferPtr() + LIBHEADERSIZE, header.length);

		if (receive_dequeue_packet_size != header.length)
		{
			wprintf(L"## ReceiveQDequeuePacketSize != Header.BySize : %d \n", receive_dequeue_packet_size);
			DebugBreak();
			break;
		}
		packet_buffer->MoveWritePosition(receive_dequeue_packet_size);

		InterlockedIncrement(&recv_message_count);

		ContentsCPacket contents_packet_buffer= ContentsCPacket::MakeContentsPacket(packet_buffer);

		OnMessage(target->session_id, contents_packet_buffer);

		if (packet_buffer->DecreaseRefCount() == 0)
		{
			CPacket::Free(packet_buffer);
		}

	}

	return true;
}

//bool LanNetworkLibraryServer::PacketProc(Session* target, CPacket* packet_buffer)
//{
//	/*if (!PacketProc(target, &packet_buffer))
//	{
//		return false;
//	}*/
//	unsigned short length = 8;
//	__int64 data;
//	*packet_buffer >> data;
//
//	//여기서 그냥 쏘자
//
//	CPacket packet_echo;
//	packet_echo << length << data;
//
//	int data_size = packet_echo.GetDataSize();
//
//	int enqueue_return;
//
//	enqueue_return = target->send_buffer.Enqueue(packet_echo.GetBufferPtr(), data_size);
//
//	if (enqueue_return != data_size)
//	{
//		//인큐 불가 상태.
//		wprintf(L"EnqueueFail in SendPacketUnicast Session Id : %lld\n", target->session_id);
//		DebugBreak();
//
//		//Disconnect(target);
//		return false;
//	}
//
//	return true;
//}

bool LanNetworkLibraryServer::Receive(Session* target)
{
	if (target->disconnect_flag == 1)
	{
		return false;
	}



	int retval;
	WSABUF recv_wsabuf[2];

	recv_wsabuf[0].buf = target->recv_buffer.GetRearBufferPtr();
	recv_wsabuf[0].len = target->recv_buffer.DirectEnqueueSize();
	recv_wsabuf[1].buf = target->recv_buffer.GetStartBufferPtr();
	recv_wsabuf[1].len = target->recv_buffer.GetFreeSize() - target->recv_buffer.DirectEnqueueSize();

	DWORD recvbytes;
	DWORD flags = 0;
	int WSARecv_error;

	ZeroMemory(&target->recv_overlapped.overlapped, sizeof(target->recv_overlapped.overlapped));
	InterlockedIncrement(&target->io_count);
	retval = WSARecv(target->sock, recv_wsabuf, 2, &recvbytes, &flags, &target->recv_overlapped.overlapped, 0);
	if (retval == SOCKET_ERROR)
	{
		WSARecv_error = WSAGetLastError();
		if (WSARecv_error == WSA_IO_PENDING)
		{

			if (target->disconnect_flag == 1)
			{
				CancelIoEx((HANDLE)target->sock, nullptr);
			}

			return true;

		}
		
		if (WSARecv_error != WSA_IO_PENDING)
		{


			if (InterlockedDecrement(&target->io_count) == 0)
			{
				Release(target);
				return false;
			}

			return true;

		}

	}

	return true;
}

bool LanNetworkLibraryServer::AddHeader(CPacket* packet_buffer)
{
	char* temp = packet_buffer->GetBufferPtr();
	temp += LIBHEADERSIZE - header_size;
	PacketHeader LibHeader;
	LibHeader.length = packet_buffer->GetDataSize();

	(*(unsigned short*)(temp)) = LibHeader.length;

	return false;
}

void LanNetworkLibraryServer::Release(Session* target)
{
	__int64 is_delete_id = target->session_id;

	//여기서 IO/Count와 릴리즈 플래그를 한 번에 
	//RELEASEFLAG 사용. 

	if (InterlockedCompareExchange(&target->io_count, RELEASEFLAG , 0) != 0)
	{
		return;
	}


	for(int i = 0; i < target->buffer_count.count; ++i)
	{
		if (target->buffer_count.buffers[i]->DecreaseRefCount() == 0)
		{

			CPacket::Free(target->buffer_count.buffers[i]);

		}
	}


	target->buffer_count.count = 0;
	target->recv_buffer.ClearBuffer();

	ClearSendBuffer(target);
	closesocket(target->sock);

	

	

	index_list.Free(target->index);

	InterlockedDecrement(&session_num);

	OnRelease(is_delete_id);

	return;
}



int* LanNetworkLibraryServer::FindEmptySessionPTR()
{
	Profile a(L"Find Empty Session");

	int* temp_index = index_list.Alloc();

	return temp_index;


}

void LanNetworkLibraryServer::ClearSendBuffer(Session* target)
{
	while (1)
	{
		CPacket* t;

		if (target->send_buffer.Dequeue(&t) == false)
		{
			break;
		}
		if (t->DecreaseRefCount() == 0)
		{
			CPacket::Free(t);
		}
	}

}

int LanNetworkLibraryServer::FindSession(__int64 session_ID)
{
	return (session_ID >> 48);
}

int LanNetworkLibraryServer::GetAcceptTPS()
{
	return accept_TPS;
}

int LanNetworkLibraryServer::GetRecvMessageTPS()
{
	return recv_message_TPS;
}

int LanNetworkLibraryServer::GetSendMessageTPS()
{
	return send_message_TPS;
}
