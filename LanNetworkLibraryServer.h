#pragma once

#include <winsock2.h>
#include "ObjectFreeList.h"
#include "ContentsCPacket.h"


#define RECV 10
#define SEND 20




struct Session;

class LanNetworkLibraryServer
{
public:
	LanNetworkLibraryServer();

	virtual ~LanNetworkLibraryServer();

	

	bool Start(const char* server_IP, unsigned int  server_port, unsigned int threads_count, unsigned int concurrent_threads, unsigned int nagle, unsigned int sessions , unsigned int header_size);
	bool Stop();
	int GetSessionCount();
	bool Disconnect(__int64 session_ID);

	bool SendPacket(__int64 session_ID, ContentsCPacket send_packet);
	bool SendPost(Session* Target);

	bool ReceiveFirst(Session* new_session);
	bool RecvProc(Session* target);
	//bool PacketProc(Session* target, CPacket* packet_buffer);
	bool Receive(Session* target);
	bool AddHeader(CPacket* packet_buffer);
	void Release(Session* target);


	void ReleasePTR(Session* target);

	int FindEmptySession();
	int FindSession(__int64 session_ID);
	int* FindEmptySessionPTR();


	static unsigned int WINAPI AcceptThread(LPVOID this_ptr);
	static unsigned int WINAPI WorkerThread(LPVOID this_ptr);
	static unsigned int WINAPI MonitorThread(LPVOID this_ptr);
	virtual bool OnConnectionRequest(const wchar_t* server_IP, unsigned short server_port) = 0;
	virtual void OnAccept(const wchar_t* server_IP, unsigned short server_port, __int64 session_ID) = 0;
	virtual void OnRelease(__int64 session_ID) = 0;

	virtual void OnMessage(__int64 session_ID, ContentsCPacket send_packet) = 0;

	virtual void OnError(int errorcode, const wchar_t* error_log) = 0;

	int GetAcceptTPS();
	int GetRecvMessageTPS();
	int GetSendMessageTPS();



	unsigned int accept_TPS;
	unsigned int recv_message_TPS;
	unsigned int send_message_TPS;

	unsigned int accept_count;
	unsigned int recv_message_count;
	long send_message_count;


	unsigned int max_session;
	unsigned int session_num;
	unsigned int threads_num;
	__int64 unique_id;

	__int64 recv_time;
	unsigned int recv_call;

	__int64 send_proc_time;
	unsigned int send_proc_call;

	__int64 send_call_time;
	unsigned int send_call;

	__int64 alloc_call_time;
	unsigned int alloc_call;

	__int64 free_call_time;
	unsigned int free_call;

	__int64 buf_count_total[200][2];



	unsigned int header_size;




	HANDLE handle_iocp;
	SOCKET listen_sock;
	HANDLE* threads;
	HANDLE accept_thread;
	HANDLE monitor_thread;
	Session* session_array;

	TObjectFreeList<int> index_list;




};

