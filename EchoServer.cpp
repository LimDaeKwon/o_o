#include "EchoServer.h"
#include "ContentsCPacket.h"

EchoServer::EchoServer()
{

}

EchoServer::~EchoServer()
{

}

bool EchoServer::OnConnectionRequest(const wchar_t* server_IP, unsigned short server_port)
{
	return false;
}

void EchoServer::OnAccept(const wchar_t* server_IP, unsigned short server_port, __int64 session_ID)
{
	ContentsCPacket login_packet = ContentsCPacket::MakeContentsPacket();

	__int64 login_data = 0x7fffffffffffffff;
	login_packet << login_data;

	SendPacket(session_ID, login_packet);


}

void EchoServer::OnRelease(__int64 session_ID)
{
	//wprintf(L"Session Release ID : %lld \n", session_ID);
}

void EchoServer::OnMessage(__int64 session_ID, ContentsCPacket contents_send_packet)
{

	__int64 data;
	//ContentsCPacket contents_send_packet = ContentsCPacket::MakeContentsPacket(send_packet);
	
	contents_send_packet >> data;

	// 
	//여기서 그냥 쏘자
	
	//CPacket* packet_echo = serialize_list.Alloc();

	ContentsCPacket packet_echo = ContentsCPacket::MakeContentsPacket();


	packet_echo << data;

	SendPacket(session_ID, packet_echo);



}

void EchoServer::OnError(int errorcode, const wchar_t* error_log)
{

}
