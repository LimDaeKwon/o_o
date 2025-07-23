// NetworkLibrary.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include "EchoServer.h"
#include "windows.h"
#include "conio.h"
#include "Profiler.h"
#include "CrashDump.h"
#include "queue"


CrashDump zz;


#pragma comment(lib, "winmm.lib")

#define FILENAME "ThreadSetting.config"


enum thread_setting_enum
{
	IP , PORT, THREADS, CONCURRENT, NAGLE, SESSIONS, HEADERSIZE
};


int max_accept_TPS = -1;
int min_accept_TPS = INT_MAX;
int max_recv_message_TPS = -1;
int min_recv_message_TPS = INT_MAX;
int max_send_message_TPS = -1;
int min_send_message_TPS = INT_MAX;
int TPS_count = 0;
unsigned int GlobalChecksum;
__int64 total_accept_TPS = 0;

__int64 total_recv_message_TPS = 0;

__int64 total_send_message_TPS = 0;



bool ParseThreadDataFile(const char* file_name);
void SaveTPS(EchoServer* echo_instance);
void TPSReset();

char ThreadData[7][200];

int main()
{

	timeBeginPeriod(1);

	InitProfile();


	ParseThreadDataFile(FILENAME);
	EchoServer echo_instance;
	echo_instance.Start(ThreadData[IP], atoi(ThreadData[PORT]), atoi(ThreadData[THREADS]), atoi(ThreadData[CONCURRENT]), atoi(ThreadData[NAGLE]), atoi(ThreadData[SESSIONS]), atoi(ThreadData[HEADERSIZE]));

	

	char start_key = _getch();
	wprintf(L"TPS Counting Start");


	while (start_key == 'e' || start_key == 'E')
	{
		SaveTPS(&echo_instance);

		if (_kbhit())
		{
			char key = _getch();

			if (key == 'q' || key == 'Q')
			{
				//종료
				echo_instance.Stop();


			}

			if (key == 'c' || key == 'C')
			{
				//초기화

				ProfileReset();

				TPSReset();


			}

		}



		Sleep(1000);
	}

	Sleep(INFINITE);





	//Sleep(INFINITE);



	ParseThreadDataFile(FILENAME);
	//echo.start(~~~~~~~~~~)

	timeEndPeriod(1);

}


bool ParseThreadDataFile(const char* file_name)
{
	FILE* worker_information;
	int file_size;
	char* file_buffer;

	if (fopen_s(&worker_information, file_name, "rb") == 0)
	{
		if (worker_information == NULL)
		{
			return false;
		}


		fseek(worker_information, 0, SEEK_END);
		file_size = ftell(worker_information);
		rewind(worker_information);


		file_buffer = (char*)malloc(file_size);
		if (file_buffer == NULL) {
			printf("Error: Memory allocation failed.\n");
			DebugBreak();
			return false;
		}

		size_t bytesRead = fread_s(file_buffer, file_size, 1, file_size, worker_information);
		if (bytesRead != file_size)
		{

			printf("Error: Failed to read entire file. Expected %d bytes, Read: %zu bytes.\n", file_size, bytesRead);

			return false;
		}

		int start_position = 0;
		int end_postiion = 0;

		//시작위치 + 0d 0a 넘기기. 

		for (int i = 0; i < file_size; ++i)
		{
			if (file_buffer[i] == '{')
			{
				start_position = i + 2;
				break;
			}
		}

		for (int i = file_size-1; i >= 0; --i)
		{
			

			if (file_buffer[i] == '}')
			{
				end_postiion = i;
				break;
			}
		}




		
		int Index = 0;

		int size;
		for (int i = start_position; i < end_postiion; ++i)
		{


			if (file_buffer[i] == ':')
			{
				for (int j = i + 1; ; ++j)
				{
					if (file_buffer[j] == 0x0d)
					{
						size = j - i -1;
						memcpy_s(ThreadData[Index], size, file_buffer + i + 1, size);
						ThreadData[Index][size] = '\0';
						Index++;
						break;
					}
				}
				start_position = i + size;
			}
		}



		free(file_buffer);

		fclose(worker_information);
	}





	return true;

}

void SaveTPS(EchoServer* echo_instance)
{
	int temp_accept = echo_instance->GetAcceptTPS();
	int temp_recv = echo_instance->GetRecvMessageTPS();
	int temp_send = echo_instance->GetSendMessageTPS();

	if (max_accept_TPS < temp_accept)
	{
		max_accept_TPS = temp_accept;
	}
	if (min_accept_TPS > temp_accept)
	{
		min_accept_TPS = temp_accept;
	}

	if (max_recv_message_TPS < temp_recv)
	{
		max_recv_message_TPS = temp_recv;
	}
	if (min_recv_message_TPS > temp_recv)
	{
		min_recv_message_TPS = temp_recv;
	}

	if (max_send_message_TPS < temp_send)
	{
		max_send_message_TPS = temp_send;
	}
	if (min_send_message_TPS > temp_send)
	{
		min_send_message_TPS = temp_send;
	}

	total_accept_TPS += temp_accept;

	total_recv_message_TPS += temp_recv;

	total_send_message_TPS += temp_send;


	TPS_count++;

	if (TPS_count == 120)
	{
		//저장

		ProfileDataOutText(L"ProfileData.txt");


		FILE* TPS_file;

		LARGE_INTEGER Freq;
		QueryPerformanceFrequency(&Freq);

		if (_wfopen_s(&TPS_file, L"TPS_Data.txt", L"wb") != 0)
		{
			wprintf(L"File Open Failed\n");
			
		}


		fwprintf(TPS_file,
			L"---------------------------------------------------------------------------------------------------------------------\n"
			L"%20s  %20s  %20s  %25s  %20s \n"
			L"---------------------------------------------------------------------------------------------------------------------\n",
			L"Name", L"Average", L"Min", L"Max", L"Call");


		fwprintf(TPS_file,
			L"%20s  %20s  %20s  %20s  %10s\n",
			L"accept_TPS",
			std::to_wstring((total_accept_TPS - min_accept_TPS - max_accept_TPS) / (TPS_count - 2)).c_str(),
			std::to_wstring(min_accept_TPS).c_str(),
			std::to_wstring(max_accept_TPS).c_str(),
			std::to_wstring(TPS_count).c_str());
		fwprintf(TPS_file,
			L"%20s  %20s  %20s  %20s  %10s\n",
			L"recv_message_TPS",
			std::to_wstring((total_recv_message_TPS - min_recv_message_TPS - max_recv_message_TPS) / (TPS_count - 2)).c_str(),
			std::to_wstring(min_recv_message_TPS).c_str(),
			std::to_wstring(max_recv_message_TPS).c_str(),
			std::to_wstring(TPS_count).c_str());
		fwprintf(TPS_file,
			L"%20s  %20s  %20s  %20s  %10s\n",
			L"send_message_TPS",
			std::to_wstring((total_send_message_TPS - min_send_message_TPS - max_send_message_TPS) / (TPS_count - 2)).c_str(),
			std::to_wstring(min_send_message_TPS).c_str(),
			std::to_wstring(max_send_message_TPS).c_str(),
			std::to_wstring(TPS_count).c_str());

		fwprintf(TPS_file,
			L"%20s  %20s  %10s\n",
			L"recv_time",
			std::to_wstring((double)(echo_instance->recv_time) / (echo_instance->recv_call) / Freq.QuadPart * 1000000).c_str(),
			std::to_wstring(echo_instance->recv_call).c_str());
		
		fwprintf(TPS_file,
			L"%20s  %20s  %10s\n",
			L"send_proc",
			std::to_wstring((double)(echo_instance->send_proc_time) / (echo_instance->send_proc_call) / Freq.QuadPart * 1000000).c_str(),
			std::to_wstring(echo_instance->send_proc_call).c_str());

		fwprintf(TPS_file,
			L"%20s  %20s  %10s\n",
			L"send_call",
			std::to_wstring((double)(echo_instance->send_call_time) / (echo_instance->send_call) / Freq.QuadPart * 1000000).c_str(),
			std::to_wstring(echo_instance->send_call).c_str());

		fwprintf(TPS_file,
			L"%20s  %20s  %10s\n",
			L"alloc",
			std::to_wstring((double)(echo_instance->alloc_call_time) / (echo_instance->alloc_call) / Freq.QuadPart * 1000000).c_str(),
			std::to_wstring(echo_instance->alloc_call).c_str());

		fwprintf(TPS_file,
			L"%20s  %20s  %10s\n",
			L"free",
			std::to_wstring((double)(echo_instance->free_call_time) / (echo_instance->free_call) / Freq.QuadPart * 1000000).c_str(),
			std::to_wstring(echo_instance->free_call).c_str());

		for (int i = 0; i < 200; i++)
		{
			if (echo_instance->buf_count_total[i][0] == 0)
			{
				continue;
			}

			fwprintf(TPS_file,
				L"%20s  %20s  %10s\n",
				std::to_wstring(i).c_str(),
				std::to_wstring((double)(echo_instance->buf_count_total[i][1]) / (echo_instance->buf_count_total[i][0]) / Freq.QuadPart * 1000000).c_str(),
				std::to_wstring(echo_instance->buf_count_total[i][0]).c_str());


		}



		fwprintf(TPS_file, L"---------------------------------------------------------------------------------------------------------------------\n");
		fclose(TPS_file);

		DebugBreak();

	}

}

void TPSReset()
{
	max_accept_TPS = -1;
	min_accept_TPS = INT_MAX;
	max_recv_message_TPS = -1;
	min_recv_message_TPS = INT_MAX;
	max_send_message_TPS = -1;
	min_send_message_TPS = INT_MAX;
	TPS_count = 0;

	total_accept_TPS = 0;

	total_recv_message_TPS = 0;

	total_send_message_TPS = 0;

}
