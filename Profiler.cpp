#include "Profiler.h"

DWORD TLS_index;

LARGE_INTEGER Freq;

void ProfileBegin(const WCHAR* szName)
{
	ProfileManagement* ProfileManage = (ProfileManagement*)TlsGetValue(TLS_index);
	if (ProfileManage == NULL)
	{
		ProfileManage = new ProfileManagement[MAXINDEX];
		memset(ProfileManage, 0, sizeof(ProfileManagement) * MAXINDEX);

		TlsSetValue(TLS_index, ProfileManage);
		ProfileReset();
	}


	int i;
	for (i = 0; i < MAXINDEX; i++)
	{
		//�̹� ��ϵ� �Լ����
		if (ProfileManage[i].Flag == 1)
		{
			if (wcscmp(ProfileManage[i].szName, szName) == 0)
			{
				LARGE_INTEGER Start;
				QueryPerformanceCounter(&Start);
				ProfileManage[i].StartTime = Start;
				ProfileManage[i].Call++;
				if (InterlockedExchange(&ProfileManage[i].begin_flag, 1) == 1)
				{
					DebugBreak();
				}


				return;
			}
		}
		else
		{
			break;
		}
	}

	//ù ���
	LARGE_INTEGER Start;
	ProfileManage[i].Flag = 1;
	wmemcpy(ProfileManage[i].szName, szName, wcslen(szName));
	QueryPerformanceCounter(&Start);
	ProfileManage[i].StartTime = Start;
	ProfileManage[i].Call++;
	ProfileManage[i].Min[0] = INT_MAX;
	ProfileManage[i].Max[0] = -1;

	ProfileManage[i].Min[1] = INT_MAX;
	ProfileManage[i].Max[1] = -1;
	if (InterlockedExchange(&ProfileManage[i].begin_flag, 1) == 1)
	{
		DebugBreak();
	}



}
void ProfileEnd(const WCHAR* szName)
{
	ProfileManagement* ProfileManage = (ProfileManagement*)TlsGetValue(TLS_index);

	int i;

	for (i = 0; i < MAXINDEX; i++)
	{

		if (wcscmp(ProfileManage[i].szName, szName) == 0)
		{
			if (InterlockedExchange(&ProfileManage[i].begin_flag, 0) == 0)
			{
				DebugBreak();
			}

			LARGE_INTEGER End;
			LARGE_INTEGER ElapsedTime;
			QueryPerformanceCounter(&End);
			ElapsedTime.QuadPart = End.QuadPart - ProfileManage[i].StartTime.QuadPart;

			ProfileManage[i].TotalTime += ElapsedTime.QuadPart;

			if (ElapsedTime.QuadPart < ProfileManage[i].Min[0])
			{
				ProfileManage[i].Min[1] = ProfileManage[i].Min[0];
				ProfileManage[i].Min[0] = ElapsedTime.QuadPart;
			}
			else if (ElapsedTime.QuadPart < ProfileManage[i].Min[1])
			{
				ProfileManage[i].Min[1] = ElapsedTime.QuadPart;
			}

			if (ElapsedTime.QuadPart > ProfileManage[i].Max[0])
			{
				ProfileManage[i].Max[1] = ProfileManage[i].Max[0];
				ProfileManage[i].Max[0] = ElapsedTime.QuadPart;
			}
			else if (ElapsedTime.QuadPart > ProfileManage[i].Max[1])
			{
				ProfileManage[i].Max[1] = ElapsedTime.QuadPart;
			}
			break;
		}

	}

}

void ProfileDataOutText(const WCHAR* szFileName)
{
	ProfileManagement* ProfileManage = (ProfileManagement*)TlsGetValue(TLS_index);

	WCHAR filename[MAX_PATH];
	wsprintf(filename, L"%s%02d.txt",
		szFileName, GetCurrentThreadId());


	FILE* WriteFile;

	if (_wfopen_s(&WriteFile, filename, L"wb") != 0)
	{
		wprintf(L"File Open Failed\n");
		return;
	}


	fwprintf(WriteFile,
		L"---------------------------------------------------------------------------------------------------------------------\n"
		L"%20s  %20s  %20s  %25s  %20s \n"
		L"---------------------------------------------------------------------------------------------------------------------\n",
		L"Name", L"Average", L"Min", L"Max", L"Call");

	for (int i = 0; i < MAXINDEX; i++)
	{
		if (ProfileManage[i].Flag == 1)
		{
			fwprintf(WriteFile,
				L"%20s  %20s��  %20s��  %20s��  %10s\n",
				ProfileManage[i].szName,
				std::to_wstring((double)(ProfileManage[i].TotalTime - ProfileManage[i].Min[0] - ProfileManage[i].Max[0] - ProfileManage[i].Min[1] - ProfileManage[i].Max[1]) / (ProfileManage[i].Call - 4) / Freq.QuadPart * 1000000).c_str(),
				std::to_wstring((double)((ProfileManage[i].Min[0] + ProfileManage[i].Min[1]) / (double)2) / Freq.QuadPart * 1000000).c_str(),
				std::to_wstring((double)((ProfileManage[i].Max[0] + ProfileManage[i].Max[1]) / (double)2) / Freq.QuadPart * 1000000).c_str(),
				std::to_wstring(ProfileManage[i].Call - 4).c_str());
		}
		else
		{
			break;
		}
	}
	fwprintf(WriteFile, L"---------------------------------------------------------------------------------------------------------------------\n");



	wprintf(L"WriteFileComplete %d\n", GetCurrentThreadId());

	fclose(WriteFile);

}
void ProfileReset()
{
	ProfileManagement* ProfileManage = (ProfileManagement*)TlsGetValue(TLS_index);


	for (int i = 0; i < MAXINDEX; i++)
	{
		if (ProfileManage[i].Flag == 1)
		{
			ProfileManage[i].TotalTime = 0;
			ProfileManage[i].Call = 0;
			ProfileManage[i].Min[0] = INT_MAX;
			ProfileManage[i].Max[0] = -1;
			ProfileManage[i].Min[1] = INT_MAX;
			ProfileManage[i].Max[1] = -1;
		}
		else
		{
			break;
		}
	}

	wprintf(L"ResetComplete %d\n", GetCurrentThreadId());


}

void InitProfile()
{
	TLS_index = TlsAlloc();
	QueryPerformanceFrequency(&Freq);
}