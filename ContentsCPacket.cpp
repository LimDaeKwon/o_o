#include "ContentsCPacket.h"

ContentsCPacket::ContentsCPacket()
{


}





ContentsCPacket::ContentsCPacket(CPacket* alloc_buffer)
{
	packet_buffer = alloc_buffer;

	packet_buffer->IncreaseRefCount();

	
}

ContentsCPacket::ContentsCPacket(ContentsCPacket& copy)
{
	packet_buffer = copy.packet_buffer;

	packet_buffer->IncreaseRefCount();

}

//ContentsCPacket& ContentsCPacket::operator=(CPacket* alloc_buffer)
//{
//	packet_buffer = alloc_buffer;
//
//	packet_buffer->IncreaseRefCount();
//
//	
//
//	return *this; // ��ü �ڽ��� ���� ��ȯ
//}

//ContentsCPacket& ContentsCPacket::operator=(ContentsCPacket* alloc_buffer)
//{
//	packet_buffer = alloc_buffer->packet_buffer;
//
//	packet_buffer->IncreaseRefCount();
//
//	
//
//	return *this; // ��ü �ڽ��� ���� ��ȯ
//}

CPacket* ContentsCPacket::MakeContentsPacket()
{

	return CPacket::Alloc();
}

CPacket* ContentsCPacket::MakeContentsPacket(CPacket* other)
{

	return other;
}

CPacket* ContentsCPacket::MakeContentsPacket(ContentsCPacket* other)
{
	
	return other->packet_buffer;
}


ContentsCPacket::~ContentsCPacket()
{
	if (packet_buffer->DecreaseRefCount() == 0)
	{
		CPacket::Free(packet_buffer);
	}
}



ContentsCPacket& ContentsCPacket::operator<<(unsigned char Value)
{
	*packet_buffer << Value;

	return *this;
	// TODO: ���⿡ return ���� �����մϴ�.
}

ContentsCPacket& ContentsCPacket::operator<<(char Value)
{
	*packet_buffer << Value;

	return *this;
	// TODO: ���⿡ return ���� �����մϴ�.
}

ContentsCPacket& ContentsCPacket::operator<<(short Value)
{
	*packet_buffer << Value;

	return *this;
	// TODO: ���⿡ return ���� �����մϴ�.
}

ContentsCPacket& ContentsCPacket::operator<<(unsigned short Value)
{
	*packet_buffer << Value;

	return *this;
	// TODO: ���⿡ return ���� �����մϴ�.
}

ContentsCPacket& ContentsCPacket::operator<<(int Value)
{
	*packet_buffer << Value;

	return *this;
}

ContentsCPacket& ContentsCPacket::operator<<(long Value)
{
	*packet_buffer << Value;

	return *this;
}

ContentsCPacket& ContentsCPacket::operator<<(unsigned int Value)
{
	*packet_buffer << Value;
	return *this;
}

ContentsCPacket& ContentsCPacket::operator<<(float Value)
{
	*packet_buffer << Value;

	return *this;
}

ContentsCPacket& ContentsCPacket::operator<<(__int64 Value)
{
	*packet_buffer << Value;

	return *this;
}

ContentsCPacket& ContentsCPacket::operator<<(double Value)
{
	*packet_buffer << Value;

	return *this;
}


//-----------------------------------------------------------------
//(*(unsigned short*)(SerializeBuffer + WritePosition)) = Value;
ContentsCPacket& ContentsCPacket::operator>>(unsigned char& Value)
{
	*packet_buffer >> Value;

	return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(char& Value)
{
	*packet_buffer >> Value;


	return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(short& Value)
{
	*packet_buffer >> Value;



	return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(unsigned short& Value)
{
	*packet_buffer >> Value;



	return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(int& Value)
{
	*packet_buffer >> Value;



	return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(unsigned int& Value)
{
	*packet_buffer >> Value;



	return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(float& Value)
{
	*packet_buffer >> Value;


	return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(__int64& Value)
{
	*packet_buffer >> Value;



	return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(double& Value)
{
	*packet_buffer >> Value;

	return *this;
}