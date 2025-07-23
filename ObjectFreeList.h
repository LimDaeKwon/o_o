#pragma once
#include "Windows.h"
#include <new.h>



extern unsigned int GlobalChecksum;
#define ADRMASK 0x00007fffffffffff
#define TAGMASK 0xffff800000000000
#define TAGOFFSET 47


#define DEBUG
#ifdef DEBUG



template <class DATA>
class TObjectFreeList
{

public:

	struct BLOCK_NODE
	{
		unsigned int UnderflowCheckSum;
		DATA Data;
		unsigned int OverflowCheckSum;
		BLOCK_NODE* Next;
	};

	TObjectFreeList(int BlockNum, bool PlacementNew = false) : IsPlacementNew(PlacementNew), Checksum(GlobalChecksum++), TopNode(NULL), UseCount(0)
	{
		BLOCK_NODE* FirstAlloc = new BLOCK_NODE;

		ChecksumPosition = (__int64)&FirstAlloc->Data - (__int64)&FirstAlloc->UnderflowCheckSum;

		delete FirstAlloc;

		if (BlockNum == 0)
		{
			Capacity = BlockNum;
			return;
		}

		for (int i = 0; i < BlockNum; ++i)
		{
			BLOCK_NODE* Temp = (BLOCK_NODE*)malloc(sizeof(BLOCK_NODE));;

			if (!IsPlacementNew)
			{
				new (&Temp->Data) DATA;
			}

			Temp->OverflowCheckSum = Checksum;
			Temp->UnderflowCheckSum = Checksum;
			Push(Temp);
		}

		Capacity = BlockNum;
	}

	virtual	~TObjectFreeList()
	{
		BLOCK_NODE* DeleteNode;

		for (int i = 0; i < Capacity; ++i)
		{
			DeleteNode = TopNode;
			TopNode = TopNode->Next;
			delete DeleteNode;
		}
	}

	__int64 MaskNewTag(__int64 LocalTop, __int64 MaskNewNode)
	{
		__int64 Tag = InterlockedIncrement(&TagIndex);
		MaskNewNode |= (Tag << TAGOFFSET);

		return MaskNewNode;
	}


	BLOCK_NODE* UnMaskTag(__int64 HeadNode)
	{
		HeadNode &= ADRMASK;
		return (BLOCK_NODE*)HeadNode;
	}

	void Push(BLOCK_NODE* NewTop)
	{
		BLOCK_NODE* LocalTop;
		__int64 MaskNewTop;
		while (1)
		{
			LocalTop = TopNode;
			MaskNewTop = MaskNewTag((__int64)LocalTop, (__int64)NewTop);
			NewTop->Next = LocalTop;

			if ((__int64)LocalTop == InterlockedCompareExchange64((__int64*)&TopNode, (__int64)MaskNewTop, (__int64)LocalTop))
			{
				break;
			}
		}
	}

	DATA* Alloc(void)
	{

		BLOCK_NODE* LocalTop;
		BLOCK_NODE* UnMaskTop;
		BLOCK_NODE* NewTop;


		while (1)
		{
			LocalTop = TopNode;
			if (LocalTop == nullptr)
			{

				BLOCK_NODE* NewNode = new BLOCK_NODE;

				NewNode->OverflowCheckSum = Checksum;
				NewNode->UnderflowCheckSum = Checksum;

				InterlockedIncrement(&UseCount);
				InterlockedIncrement(&Capacity);

				return &NewNode->Data;

			}

			UnMaskTop = UnMaskTag((__int64)LocalTop);
			NewTop = UnMaskTop->Next;

			if ((__int64)LocalTop == InterlockedCompareExchange64((__int64*)&TopNode, (__int64)NewTop, (__int64)LocalTop))
			{
				break;
			}
		}

		if (IsPlacementNew)
		{
			new (&UnMaskTop->Data) DATA;
		}

		InterlockedIncrement(&UseCount);

		return &UnMaskTop->Data;

	}


	bool	Free(DATA* Data)
	{


		char* MovePointer = (char*)Data;

		MovePointer -= ChecksumPosition;

		BLOCK_NODE* Temp = (BLOCK_NODE*)MovePointer;

		if (Temp->OverflowCheckSum != Checksum)
		{
			//wprintf(L"Overflow \n");
			DebugBreak();
		}
		if (Temp->UnderflowCheckSum != Checksum)
		{
			//wprintf(L"Underflow \n");
			DebugBreak();
		}

		if (IsPlacementNew)
		{
			Temp->Data.~DATA();
		}

		Push(Temp);

		InterlockedDecrement(&UseCount);
		return true;
	}


	int		GetCapacityCount(void) { return Capacity; }

	int		GetUseCount(void) { return UseCount; }

	unsigned int Checksum;

private:
	BLOCK_NODE* TopNode;
	__int64 ChecksumPosition;
	bool IsPlacementNew;
	long Capacity;
	long UseCount;

	unsigned long long TagIndex;

};

//template <class DATA>
//thread_local TObjectFreeList<DATA> pool;


#else

template <class DATA>
class TObjectFreeList
{

public:

	struct BLOCK_NODE
	{
		DATA Data;
		BLOCK_NODE* Next;
	};

	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Parameters:	(int) �ʱ� �� ����.
	//				(bool) Alloc �� ������ / Free �� �ı��� ȣ�� ����
	// Return:
	//////////////////////////////////////////////////////////////////////////
	TObjectFreeList(int BlockNum, bool PlacementNew = false) : IsPlacementNew(PlacementNew), TopNode(NULL), UseCount(0)
	{

		if (BlockNum == 0)
		{
			Capacity = BlockNum;
			return;
		}

		for (int i = 0; i < BlockNum; ++i)
		{
			BLOCK_NODE* Temp = (BLOCK_NODE*)malloc(sizeof(BLOCK_NODE));;

			if (!IsPlacementNew)
			{
				new (&Temp->Data) DATA;
			}
			Push(Temp);
		}

		Capacity = BlockNum;
	}

	virtual	~TObjectFreeList()
	{
		BLOCK_NODE* DeleteNode;

		for (int i = 0; i < Capacity; ++i)
		{
			DeleteNode = TopNode;
			TopNode = TopNode->Next;
			delete DeleteNode;
		}
	}

	__int64 MaskNewTag(__int64 LocalTop, __int64 MaskNewNode)
	{
		__int64 Tag = InterlockedIncrement(&TagIndex);
		MaskNewNode |= (Tag << 47);
		return MaskNewNode;
	}


	BLOCK_NODE* UnMaskTag(__int64 HeadNode)
	{
		HeadNode &= ADRMASK;
		return (BLOCK_NODE*)HeadNode;
	}

	void Push(BLOCK_NODE* NewTop)
	{
		BLOCK_NODE* LocalTop;
		__int64 MaskNewTop;

		while (1)
		{
			LocalTop = TopNode;
			MaskNewTop = MaskNewTag((__int64)LocalTop, (__int64)NewTop);
			NewTop->Next = LocalTop;

			if ((__int64)LocalTop == InterlockedCompareExchange64((__int64*)&TopNode, (__int64)MaskNewTop, (__int64)LocalTop))
			{
				break;
			}
		}

	}


	BLOCK_NODE* Pop()
	{
		BLOCK_NODE* LocalTop;
		BLOCK_NODE* UnMaskTop;
		BLOCK_NODE* NewTop;

		while (1)
		{

			LocalTop = TopNode;
			UnMaskTop = UnMaskTag((__int64)LocalTop);
			NewTop = UnMaskTop->Next;

			if ((__int64)LocalTop == InterlockedCompareExchange64((__int64*)&TopNode, (__int64)NewTop, (__int64)LocalTop))
			{
				break;
			}
		}

		return UnMaskTop;
	}

	DATA* Alloc(void)
	{

		BLOCK_NODE* LocalTop;
		BLOCK_NODE* UnMaskTop;
		BLOCK_NODE* NewTop;

		while (1)
		{
			LocalTop = TopNode;
			if (LocalTop == nullptr)
			{
				BLOCK_NODE* NewNode = new BLOCK_NODE;

				InterlockedIncrement(&UseCount);
				InterlockedIncrement(&Capacity);

				return &NewNode->Data;
			}

			UnMaskTop = UnMaskTag((__int64)LocalTop);
			NewTop = UnMaskTop->Next;

			if ((__int64)LocalTop == InterlockedCompareExchange64((__int64*)&TopNode, (__int64)NewTop, (__int64)LocalTop))
			{
				break;
			}
		}

		if (IsPlacementNew)
		{
			new (&UnMaskTop->Data) DATA;
		}
		InterlockedIncrement(&UseCount);

		return &UnMaskTop->Data;

	}


	bool	Free(DATA* Data)
	{
		BLOCK_NODE* Temp = (BLOCK_NODE*)Data;

		if (IsPlacementNew)
		{
			Temp->Data.~DATA();
		}

		Push(Temp);

		InterlockedDecrement(&UseCount);
		return true;
	}


	int		GetCapacityCount(void) { return Capacity; }

	int		GetUseCount(void) { return UseCount; }

	unsigned int Checksum;

private:
	BLOCK_NODE* TopNode;
	__int64 ChecksumPosition;
	bool IsPlacementNew;
	long Capacity;
	long UseCount;

	unsigned long long TagIndex;

};


#endif

