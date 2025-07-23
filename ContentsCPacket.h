#pragma once
#include"CPacket.h"

class ContentsCPacket
{
private:
	ContentsCPacket();
	
	

public:

	ContentsCPacket(ContentsCPacket& copy);
	ContentsCPacket(CPacket* alloc_buffer);

	
	//ContentsCPacket& operator=(CPacket* alloc_buffer);

	static CPacket* MakeContentsPacket();
	static CPacket* MakeContentsPacket(CPacket* other);
	static CPacket* MakeContentsPacket(ContentsCPacket* other);

	//ContentsCPacket& operator=(ContentsCPacket* alloc_buffer);

	~ContentsCPacket();
	 

	CPacket* packet_buffer;


	//////////////////////////////////////////////////////////////////////////
	// �ֱ�.	�� ���� Ÿ�Ը��� ��� ����.
	//////////////////////////////////////////////////////////////////////////
	ContentsCPacket& operator << (unsigned char Value);
	ContentsCPacket& operator << (char Value);

	ContentsCPacket& operator << (short Value);
	ContentsCPacket& operator << (unsigned short Value);

	ContentsCPacket& operator << (int Value);
	ContentsCPacket& operator << (long Value);
	ContentsCPacket& operator << (unsigned int Value);
	ContentsCPacket& operator << (float Value);

	ContentsCPacket& operator << (__int64 Value);
	ContentsCPacket& operator << (double Value);


	//////////////////////////////////////////////////////////////////////////
	// ����.	�� ���� Ÿ�Ը��� ��� ����.
	//////////////////////////////////////////////////////////////////////////
	ContentsCPacket& operator >> (unsigned char& Value);
	ContentsCPacket& operator >> (char& Value);

	ContentsCPacket& operator >> (short& Value);
	ContentsCPacket& operator >> (unsigned short& Value);

	ContentsCPacket& operator >> (int& Value);
	ContentsCPacket& operator >> (unsigned int& Value);
	ContentsCPacket& operator >> (float& Value);

	ContentsCPacket& operator >> (__int64& Value);
	ContentsCPacket& operator >> (double& Value);





};

