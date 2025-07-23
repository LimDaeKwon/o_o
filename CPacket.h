#pragma once

#define BUFFER_DEFAULT 100
#define LIBHEADERSIZE 5

#include "TLSObjectFreeList.h"



class CPacket
{
	CPacket();
	CPacket(int NewBufferSize);
	


public:
	friend class LanNetworkLibraryServer;
	friend class ContentsCPacket;
	friend class TLSObjectFreeList<CPacket>;

	/*---------------------------------------------------------------
	Packet Enum.

	----------------------------------------------------------------*/

	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Return:
	//////////////////////////////////////////////////////////////////////////
	

	virtual	~CPacket();


	//////////////////////////////////////////////////////////////////////////
	// ��Ŷ û��.
	//
	// Parameters: ����.
	// Return: ����.
	//////////////////////////////////////////////////////////////////////////
	void	Clear(void);


	//////////////////////////////////////////////////////////////////////////
	// ���� ������ ���.
	//
	// Parameters: ����.
	// Return: (int)��Ŷ ���� ������ ���.
	//////////////////////////////////////////////////////////////////////////
	int	GetBufferSize(void) { return BufferSize; }
	//////////////////////////////////////////////////////////////////////////
	// ���� ������� ������ ���.
	//
	// Parameters: ����.
	// Return: (int)������� ����Ÿ ������.
	//////////////////////////////////////////////////////////////////////////
	int		GetDataSize(void) { return DataSize; }



	//////////////////////////////////////////////////////////////////////////
	// ���� ������ ���.
	//
	// Parameters: ����.
	// Return: (char *)���� ������.
	//////////////////////////////////////////////////////////////////////////
	char* GetBufferPtr(void) { return SerializeBuffer; }


	//////////////////////////////////////////////////////////////////////////
	// ���� Pos �̵�. (���� �̵��� �ȵ�)
	// GetBufferPtr �Լ��� �̿��Ͽ� �ܺο��� ������ ���� ������ ������ ��� ���. 
	//
	// Parameters: (int) �̵� ������.
	// Return: (int) �̵��� ������.
	//////////////////////////////////////////////////////////////////////////
	int		MoveWritePosition(int Size);
	int		MoveReadPosition(int Size);

	int IncreaseRefCount();
	int DecreaseRefCount();







	/* ============================================================================= */
	// ������ �����ε�
	/* ============================================================================= */
	CPacket& operator = (CPacket& SourcePacket);

	//////////////////////////////////////////////////////////////////////////
	// �ֱ�.	�� ���� Ÿ�Ը��� ��� ����.
	//////////////////////////////////////////////////////////////////////////
	CPacket& operator << (unsigned char Value);
	CPacket& operator << (char Value);

	CPacket& operator << (short Value);
	CPacket& operator << (unsigned short Value);

	CPacket& operator << (int Value);
	CPacket& operator << (long Value);
	CPacket& operator << (unsigned int Value);
	CPacket& operator << (float Value);

	CPacket& operator << (__int64 Value);
	CPacket& operator << (double Value);


	//////////////////////////////////////////////////////////////////////////
	// ����.	�� ���� Ÿ�Ը��� ��� ����.
	//////////////////////////////////////////////////////////////////////////
	CPacket& operator >> (unsigned char& Value);
	CPacket& operator >> (char& Value);

	CPacket& operator >> (short& Value);
	CPacket& operator >> (unsigned short& Value);

	CPacket& operator >> (int& Value);
	CPacket& operator >> (unsigned int& Value);
	CPacket& operator >> (float& Value);

	CPacket& operator >> (__int64& Value);
	CPacket& operator >> (double& Value);




	//////////////////////////////////////////////////////////////////////////
	// ����Ÿ ���.
	//
	// Parameters: (char *)Dest ������. (int)Size.
	// Return: (int)������ ������.
	//////////////////////////////////////////////////////////////////////////
	int		GetData(char* Destination, int DestinationSize);

	//////////////////////////////////////////////////////////////////////////
	// ����Ÿ ����.
	//
	// Parameters: (char *)Src ������. (int)SrcSize.
	// Return: (int)������ ������.
	//////////////////////////////////////////////////////////////////////////
	int		PutData(char* Source, int SourceSize);

	//void PrintOffset();
	


protected:

	static void Free(CPacket* packet_buffer);

	static CPacket* Alloc();

	char* SerializeBuffer;

	int	BufferSize;

	long ref_count;

	int	DataSize;
	//�갡 ����

	int WritePosition;
	//�갡 ����Ʈ

	int ReadPosition;

	static TLSObjectFreeList<CPacket> serialize_list;
	
};



