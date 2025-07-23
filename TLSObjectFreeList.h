#pragma once
#include "Windows.h"
#include <new.h>

extern unsigned int GlobalChecksum;
#define ADRMASK 0x00007fffffffffff
#define TAGMASK 0xffff800000000000


#define TAGOFFSET 47

#define MAXNODECOUNT 100

template <class DATA>
class TLSObjectFreeList // 얘가 노드 풀. 
{

public:

	struct BLOCK_NODE
	{
		unsigned int UnderflowCheckSum;
		DATA Data;
		unsigned int OverflowCheckSum;
		BLOCK_NODE* Next;
		BLOCK_NODE* PoolNext;
	};




	TLSObjectFreeList(int BlockNum, bool PlacementNew = false) : IsPlacementNew(PlacementNew), Checksum(GlobalChecksum++), TopNode(NULL), PoolSize(0)
	{
		TLSIndex = TlsAlloc();
		if (TLSIndex == TLS_OUT_OF_INDEXES)
		{
			__debugbreak();
		}

		InitBlockNum = BlockNum;
		BLOCK_NODE* FirstAlloc = new (BLOCK_NODE);

		ChecksumPosition = (__int64)&FirstAlloc->Data - (__int64)&FirstAlloc->UnderflowCheckSum;

		delete FirstAlloc;

	}

	virtual	~TLSObjectFreeList()
	{
		//소멸자 코드 작성 다시 해야 함.
		/*POOL_NODE* DeleteNode;

		for (int i = 0; i < Capacity; ++i)
		{
			DeleteNode = TopNode;
			TopNode = TopNode->Next;
			delete DeleteNode;
		}*/
	}


	void PoolPush(BLOCK_NODE* NewTop)
	{

		BLOCK_NODE* NewPoolNode = NewTop;

		BLOCK_NODE* LocalTop;
		__int64 MaskNewTop = MaskNewTag((__int64)NewPoolNode);
		while (1)
		{
			LocalTop = TopNode;
		//	MaskNewTop = MaskNewTag((__int64)NewPoolNode);
			NewPoolNode->PoolNext = LocalTop;

			if ((__int64)LocalTop == InterlockedCompareExchange64((__int64*)&TopNode, (__int64)MaskNewTop, (__int64)LocalTop))
			{
				InterlockedIncrement(&PoolSize);
				break;
			}
		}
	}

	BLOCK_NODE* AllocNewBucket()
	{
		BLOCK_NODE* TempTop = nullptr;
		//버킷 단위로 할당해서 넘겨주기. 
		for (int i = 0; i < MAXNODECOUNT; ++i)
		{

			BLOCK_NODE* Temp = (BLOCK_NODE*)malloc(sizeof(BLOCK_NODE));;

			Temp->Next = TempTop;


			if (!IsPlacementNew)
			{
				new (&Temp->Data) DATA;
			}

			Temp->OverflowCheckSum = Checksum;
			Temp->UnderflowCheckSum = Checksum;

			TempTop = Temp;
		}

		return TempTop;
	}

	BLOCK_NODE* PoolAlloc(void)
	{

		BLOCK_NODE* LocalTop;
		BLOCK_NODE* UnMaskTop;
		BLOCK_NODE* NewTop;

		long size = InterlockedDecrement(&PoolSize);
		if (size < 0)
		{
			InterlockedIncrement(&PoolSize);
			return AllocNewBucket();;
		}

		while (1)
		{
			LocalTop = TopNode;

			UnMaskTop = (BLOCK_NODE*)UnMaskTag((__int64)LocalTop);
			NewTop = UnMaskTop->PoolNext;

			if ((__int64)LocalTop == InterlockedCompareExchange64((__int64*)&TopNode, (__int64)NewTop, (__int64)LocalTop))
			{
				break;
			}
		}
		return (BLOCK_NODE*)UnMaskTop;

	}

private:
	BLOCK_NODE* TopNode;
	__int64 ChecksumPosition;
	bool IsPlacementNew;
	long Capacity;
	long PoolSize;
	long InitBlockNum;
	DWORD TLSIndex;
	unsigned long long TagIndex;
	unsigned int Checksum;

public:


	struct ThreadLocalMember
	{
		BLOCK_NODE* TopNode;
		long TopCount;

		BLOCK_NODE* FreeNode;
		long FreeCount;

	};

	ThreadLocalMember* GetTLS()
	{

		ThreadLocalMember* LocalMember = (ThreadLocalMember*)TlsGetValue(TLSIndex);
		if (LocalMember == NULL)
		{
			LocalMember = new ThreadLocalMember;
			memset(LocalMember, 0, sizeof(ThreadLocalMember));


			for (int i = 0; i < InitBlockNum; ++i)
			{
				BLOCK_NODE* Temp = (BLOCK_NODE*)malloc(sizeof(BLOCK_NODE));;

				if (!IsPlacementNew)
				{
					new (&Temp->Data) DATA;
				}

				Temp->OverflowCheckSum = Checksum;
				Temp->UnderflowCheckSum = Checksum;


				Push(Temp, LocalMember);
			}

			Capacity = InitBlockNum;


			TlsSetValue(TLSIndex, LocalMember);
		}

		return LocalMember;
	}


	__int64 MaskNewTag(__int64 MaskNewNode)
	{
		__int64 Tag = InterlockedIncrement(&TagIndex);
		MaskNewNode |= (Tag << TAGOFFSET);

		return MaskNewNode;
	}

	__int64 UnMaskTag(__int64 HeadNode)
	{
		HeadNode &= ADRMASK;
		return HeadNode;
	}


	void Push(BLOCK_NODE* NewTop, ThreadLocalMember* ThreadLocal)
	{
		//메인 풀 다 찼는지

		if (ThreadLocal->TopCount == MAXNODECOUNT)
		{

			NewTop->Next = ThreadLocal->FreeNode;
			ThreadLocal->FreeNode = NewTop;
			ThreadLocal->FreeCount++;

			//반환용 풀 다 찼는지
			if (ThreadLocal->FreeCount == MAXNODECOUNT)
			{
				//반환
				PoolPush(ThreadLocal->FreeNode);
				ThreadLocal->FreeNode = nullptr;
				ThreadLocal->FreeCount = 0;
			}

			return;
		}
		NewTop->Next = ThreadLocal->TopNode;
		ThreadLocal->TopNode = NewTop;
		ThreadLocal->TopCount++;

	}


	DATA* Alloc(void)
	{

		BLOCK_NODE* NewData;

		ThreadLocalMember* ThreadLocal = GetTLS();

		if (ThreadLocal->FreeNode == nullptr)
		{
			if (ThreadLocal->TopNode == nullptr)
			{
				ThreadLocal->TopNode = PoolAlloc();
				ThreadLocal->TopCount = MAXNODECOUNT;
			}

			NewData = ThreadLocal->TopNode;
			ThreadLocal->TopNode = NewData->Next;
			ThreadLocal->TopCount--;

			if (IsPlacementNew)
			{
				new (&NewData->Data) DATA;
			}

			return &NewData->Data;

		}

		NewData = ThreadLocal->FreeNode;
		ThreadLocal->FreeNode = NewData->Next;
		ThreadLocal->FreeCount--;

		if (IsPlacementNew)
		{
			new (&NewData->Data) DATA;
		}

		//InterlockedIncrement(&PoolSize);

		return &NewData->Data;

	}

	bool	Free(DATA* Data)
	{
		ThreadLocalMember* ThreadLocal = GetTLS();

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

		Push(Temp, ThreadLocal);
		return true;
	}


	int		GetCapacityCount(void) { return Capacity; }

	int		GetUseCount(void) { return PoolSize; }




};

