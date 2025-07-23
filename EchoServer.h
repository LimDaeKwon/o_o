#pragma once

#include "LanNetworkLibraryServer.h"

//class ContentsCPacket;


class EchoServer : public LanNetworkLibraryServer
{
public:
	EchoServer();
	virtual ~EchoServer();

	virtual bool OnConnectionRequest(const wchar_t* server_IP, unsigned short server_port);
	virtual void OnAccept(const wchar_t* server_IP, unsigned short server_port, __int64 session_ID);
	virtual void OnRelease(__int64 session_ID);

	virtual void OnMessage(__int64 session_ID, ContentsCPacket send_packet);

	virtual void OnError(int errorcode, const wchar_t* error_log);



};

