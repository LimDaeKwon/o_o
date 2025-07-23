#include "MyRingBuffer.h"
#include "stdio.h"
#include "memory.h"
#include "Windows.h"

MyRingBuffer::MyRingBuffer(void)
{
	Front = 0;
	Rear = 0;
	RingBuffer = new char[DEFAULTBUFFERSIZE];
	Size = DEFAULTBUFFERSIZE;

}

MyRingBuffer::MyRingBuffer(int BufferSize)
{
	Front = 0;
	Rear = 0;
	RingBuffer = new char[BufferSize];
	Size = BufferSize;
}

MyRingBuffer::~MyRingBuffer()
{
	delete RingBuffer;
}



void MyRingBuffer::Resize(int NewSize)
{
	//�� ���� ũ��� �ű�� �����Ÿ�?
	//���� ������� ������ ũ�Ⱑ NewSize���� ũ�ٸ� �Ұ���. �ƴϸ� ����.
	if (GetUseSize() > NewSize)
	{
		//���� ������� ������ ũ�Ⱑ NewSize���� ũ�ٸ� �Ұ���. 

		return;
	}

	//�� ū ũ��� �ű�� �����Ÿ�?
	//�׳� ���� �Ű���. // �ٲ����մϴ�.
	//����ִ� ũ�⸸ŭ �� ��ť�� ���༭ �ϸ� �ǰڳ�.



	char* Temp = new char[NewSize];
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
	Size = NewSize;


}

int MyRingBuffer::GetBufferSize(void)
{
	return Size;
}
//��� 3�̰� ����Ʈ�� 0�̸� 012 �� 3���ִ°�.,
int MyRingBuffer::GetUseSize(void)
{
	int LocalFront;
	int LocalRear;

	LocalFront = Front;
	LocalRear = Rear;

	if (LocalFront > LocalRear)
	{


		return Size - LocalFront + LocalRear;
	}


	return LocalRear - LocalFront;
}
int MyRingBuffer::GetFreeSize(void)
{
	//���� �ִ� ����� 8 . ���۴� 10. �׷� 1 

	return Size - GetUseSize() - 1;
}

int MyRingBuffer::Enqueue(const char* Data, int EnqueueSize)
{

	if (GetFreeSize() < EnqueueSize)
	{

		return 0;
	}


	if (Size - Rear < EnqueueSize)
	{
		int FirstEnqueueSize = Size - Rear;
		if (memcpy_s(RingBuffer + Rear, FirstEnqueueSize, Data, FirstEnqueueSize) != 0)
		{
			int Error = GetLastError();
			wprintf(L"memcpy_s Error %d \n", Error);
			DebugBreak();
		}
		int SecondEnqueueSize = EnqueueSize - FirstEnqueueSize;
		//�׸��� ���� ��ŭ.
		if (memcpy_s(RingBuffer, SecondEnqueueSize, Data + FirstEnqueueSize, SecondEnqueueSize) != 0)
		{
			int Error = GetLastError();
			wprintf(L"memcpy_s Error %d \n", Error);
			DebugBreak();
		}
		Rear = SecondEnqueueSize;

		return EnqueueSize;
	}




	//�̰� �׳�.
	if (memcpy_s(RingBuffer + Rear, EnqueueSize, Data, EnqueueSize) != 0)
	{
		int Error = GetLastError();
		wprintf(L"memcpy_s Error %d \n", Error);
		DebugBreak();

	}
	//��� 0. �����͸� �ִ´ٸ�. 10��. 0~9 ����� 10�� �����Ѿ���. 
	//���ݾ�.

	if (Rear + EnqueueSize == GetBufferSize())
	{
		Rear = 0;
	}
	else
	{
		Rear += EnqueueSize;
	}

	return EnqueueSize;
}

int MyRingBuffer::Dequeue(char* Dest, int DequeueSize)
{

	if (GetUseSize() < DequeueSize)
	{

		return 0;

	}

	if (Size - Front < DequeueSize)
	{
		int FirstDequeueSize = Size - Front;
		if (memcpy_s(Dest, FirstDequeueSize, RingBuffer + Front, FirstDequeueSize) != 0)
		{
			int Error = GetLastError();
			wprintf(L"memcpy_s Error %d \n", Error);
			DebugBreak();
		}
		int SecondDequeueSize = DequeueSize - FirstDequeueSize;

		if (memcpy_s(Dest + FirstDequeueSize, SecondDequeueSize, RingBuffer, SecondDequeueSize) != 0)
		{
			int Error = GetLastError();
			wprintf(L"memcpy_s Error %d \n", Error);
			DebugBreak();
		}
		Front = SecondDequeueSize;

		return DequeueSize;
	}
	//�̰� �׳�.
	if (memcpy_s(Dest, DequeueSize, RingBuffer + Front, DequeueSize) != 0)
	{
		int Error = GetLastError();
		wprintf(L"memcpy_s Error %d \n", Error);
		DebugBreak();

	}
	//����Ʈ�� 0�̾���. 10�� ��. ���� ����Ʈ�� 10�̾����.


	if (Front + DequeueSize == GetBufferSize())
	{
		Front = 0;
	}
	else
	{
		Front += DequeueSize;
	}

	return DequeueSize;
}

int MyRingBuffer::Peek(char* Dest, int PeekSize)
{
	//���� ���⸸.
	//�ϴ� ���ϴ� �����ŭ �ִ��� Ȯ���ؾ���.
	if (GetUseSize() < PeekSize)
	{
		printf("������ ����� ����..\n");
		return 0;

	}

	//�˱׶��� ��� ����?
	//������? ���� ����Ʈ�� �����ϴ°�. �ѹ濡 �������� ������? ������ - ����Ʈ�� ��ť������� ���� ��.
	//���� ����Ʈ�� 70�̰� ��ť������� 40�̾�. �׷� 30������ ���� ����. �� �� 0���� ���������ŭ ���ְ� ����Ʈ �������ֱ�.
	// 30���� �ֱ� ������. ����Ʈ�� 0�̾�. ��ť�� �� . 
	//�ٵ� 70���� �����Ͱ� �ִµ�? 0���ʹϱ�. 99���� ����. 71~99 ����. 29��. 
	// 0�̰� 35�� ����. 0~34������ �����Ͱ� �ִ���. 35�� ���߰ڳ�. 
	if (Size - Front < PeekSize)
	{
		int FirstPeekSize = Size - Front;
		if (memcpy_s(Dest, FirstPeekSize, RingBuffer + Front, FirstPeekSize) != 0)
		{
			int Error = GetLastError();
			wprintf(L"memcpy_s Error %d \n", Error);
			DebugBreak();
		}
		int SecondPeekSize = PeekSize - FirstPeekSize;

		if (memcpy_s(Dest + FirstPeekSize, SecondPeekSize, RingBuffer, SecondPeekSize) != 0)
		{
			int Error = GetLastError();
			wprintf(L"memcpy_s Error %d \n", Error);
			DebugBreak();
		}
		return PeekSize;
	}
	//�̰� �׳�.
	if (memcpy_s(Dest, PeekSize, RingBuffer + Front, PeekSize) != 0)
	{
		int Error = GetLastError();
		wprintf(L"memcpy_s Error %d \n", Error);
		DebugBreak();

	}




	return PeekSize;
}

void MyRingBuffer::ClearBuffer(void)
{
	Front = 0;
	Rear = 0;

}


int MyRingBuffer::DirectEnqueueSize(void)
{

	int LocalFront;
	int LocalRear;

	LocalFront = Front;
	LocalRear = Rear;



	if (LocalRear >= LocalFront)
	{
		if (LocalFront == 0)
		{
			return Size - LocalRear - 1;
		}
		else
		{
			return Size - LocalRear;
		}

	}



	return GetFreeSize();


}

int MyRingBuffer::DirectDequeueSize(void)
{
	//����0 ����Ʈ 9 -> 1��. size - ����Ʈ �ϸ� 1.
	//
	// 
	// ���� 8 ����Ʈ 3 �̶�� �׳� 34567 5��. ���� - ����Ʈ. wmr d
	//
	//����Ʈ0 ���� 9 9��. ������ - ���� 

	int LocalFront;
	int LocalRear;
	int UseSize;

	LocalFront = Front;
	LocalRear = Rear;




	if (LocalFront > LocalRear)
	{
		UseSize = Size - LocalFront + LocalRear;
	}
	else
	{
		UseSize = LocalRear - LocalFront;
	}

	if (LocalRear < LocalFront)
	{
		if (UseSize <= Size - LocalFront)
		{


			return UseSize;
		}
		else
		{


			return Size - LocalFront;
		}
	}


	return UseSize;

}

int MyRingBuffer::MoveRear(int MoveSize)
{
	if (MoveSize <= 0)
	{
		return 0;
	}


	if (Size - Rear < MoveSize)
	{
		int FirstEnqueueSize = Size - Rear;
		int SecondEnqueueSize = MoveSize - FirstEnqueueSize;
		//�׸��� ���� ��ŭ.
		Rear = SecondEnqueueSize;
		return MoveSize;
	}

	if (Rear + MoveSize == Size)
	{
		Rear = 0;
	}
	else
	{
		Rear += MoveSize;
	}

	return MoveSize;

}

int MyRingBuffer::MoveFront(int MoveSize)
{
	if (MoveSize <= 0)
	{
		return 0;
	}


	if (Size - Front < MoveSize)
	{
		int FirstDequeueSize = Size - Front;

		int SecondDequeueSize = MoveSize - FirstDequeueSize;

		Front = SecondDequeueSize;

		return MoveSize;
	}

	//����Ʈ�� 0�̾���. 10�� ��. ���� ����Ʈ�� 10�̾����.


	if (Front + MoveSize == Size)
	{
		Front = 0;
	}
	else
	{
		Front += MoveSize;
	}

	return MoveSize;
}

char* MyRingBuffer::GetFrontBufferPtr(void)
{
	return RingBuffer + Front;
}

char* MyRingBuffer::GetRearBufferPtr(void)
{
	return RingBuffer + Rear;
}

char* MyRingBuffer::GetStartBufferPtr(void)
{
	return RingBuffer;
}