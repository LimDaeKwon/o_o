#include "CPacketQueue.h"
#include "stdio.h"
#include "memory.h"
#include "Windows.h"
#include "CPacket.h"

CPacketQueue::CPacketQueue(void) : front(0),rear(0),size(DEFAULTBUFFERSIZE)
{

	packet_queue = (CPacket**)malloc(sizeof(CPacket*) * DEFAULTBUFFERSIZE);

}

CPacketQueue::CPacketQueue(int BufferSize) : front(0), rear(0), size(BufferSize)
{

	packet_queue = (CPacket**)malloc(sizeof(CPacket*) * BufferSize);

}

CPacketQueue::~CPacketQueue()
{
	delete packet_queue;
}



void CPacketQueue::Resize(int NewSize)
{
	//�� ���� ũ��� �ű�� �����Ÿ�?
	//���� ������� ������ ũ�Ⱑ NewSize���� ũ�ٸ� �Ұ���. �ƴϸ� ����.
	//if (GetUseSize() > NewSize)
	//{
	//	//���� ������� ������ ũ�Ⱑ NewSize���� ũ�ٸ� �Ұ���. 

	//	return;
	//}

	//�� ū ũ��� �ű�� �����Ÿ�?
	//�׳� ���� �Ű���. // �ٲ����մϴ�.
	//����ִ� ũ�⸸ŭ �� ��ť�� ���༭ �ϸ� �ǰڳ�.



	/*char* Temp = new char[NewSize];
	int UseSize = GetUseSize();
	if (Dequeue(Temp, UseSize) != UseSize)
	{

		int Error = GetLastError();
		wprintf(L"Dequeue Error %d \n", Error);
		DebugBreak();


	}
	delete[] RingBuffer;

	RingBuffer = Temp;
	Front = 0;
	Rear = UseSize;
	Size = NewSize;*/


}

int CPacketQueue::GetBufferSize(void)
{
	return size;
}

//��� 3�̰� ����Ʈ�� 0�̸� 012 �� 3���ִ°�.,
int CPacketQueue::GetUseSize(void)
{
	int LocalFront;
	int LocalRear;

	LocalFront = front;
	LocalRear = rear;

	if (LocalFront > LocalRear)
	{


		return size - LocalFront + LocalRear;
	}


	return LocalRear - LocalFront;
}
int CPacketQueue::GetFreeSize(void)
{
	//���� �ִ� ����� 8 . ���۴� 10. �׷� 1 

	return size - GetUseSize() - 1;
}

int CPacketQueue::IsEmpty(void)
{
	return rear == front;
}

bool CPacketQueue::Enqueue(CPacket* Data)
{

	if (GetFreeSize() == 0)
	{

		return false;
	}

	packet_queue[rear] = Data;


	if (rear == size - 1)
	{
		rear = 0;
	}
	else
	{
		rear++;
	}

	return true;
}

bool CPacketQueue::Dequeue(CPacket** Out)
{

	if (IsEmpty())
	{

		return false;

	}
	*Out = packet_queue[front];
 	

	if (front == size -1)
	{
		front = 0;
	}
	else
	{
		front++;
	}

	return true;
}

void CPacketQueue::ClearBuffer(void)
{
	front = 0;
	rear = 0;
}


