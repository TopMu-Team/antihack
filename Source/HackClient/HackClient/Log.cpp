// Log.cpp: implementation of the CLog class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Log.h"
#include <string> // for string and to_string()
#include <iostream>

using namespace std;

CLog gLog;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLog::CLog() // OK
{
	this->m_count = 0;
}

CLog::~CLog() // OK
{
	for(int n=0;n < this->m_count;n++)
	{
		if(this->m_LogInfo[n].Active != 0)
		{
			CloseHandle(this->m_LogInfo[n].File);
		}
	}
}

void CLog::AddLog(BOOL active,char* directory) // OK
{
	if(this->m_count < 0 || this->m_count >= MAX_LOG)
	{
		return;
	}

	LOG_INFO* lpInfo = &this->m_LogInfo[this->m_count++];

	lpInfo->Active = active;

	strcpy_s(lpInfo->Directory,directory);

	if(lpInfo->Active != 0)
	{
		CreateDirectory(lpInfo->Directory,0);

		SYSTEMTIME time;

		GetLocalTime(&time);

		lpInfo->Day = time.wDay;

		lpInfo->Month = time.wMonth;

		lpInfo->Year = time.wYear;

		wsprintf(lpInfo->Filename,".\\%s\\%04d-%02d-%02d.log",lpInfo->Directory,lpInfo->Year,lpInfo->Month,lpInfo->Day);

		lpInfo->File = CreateFile(lpInfo->Filename,GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);

		if(lpInfo->File == INVALID_HANDLE_VALUE)
		{
			lpInfo->Active = 0;
			return;
		}

		if(SetFilePointer(lpInfo->File,0,0,FILE_END) == INVALID_SET_FILE_POINTER)
		{
			lpInfo->Active = 0;
			CloseHandle(lpInfo->File);
			return;
		}
	}
}

void CLog::Output(eLogType type,char* text,...) // OK
{
	if(type < 0 || type >= this->m_count)
	{
		return;
	}

	LOG_INFO* lpInfo = &this->m_LogInfo[type];

	if(lpInfo->Active == 0)
	{
		return;
	}

	SYSTEMTIME time;

	GetLocalTime(&time);

	if(time.wDay != lpInfo->Day || time.wMonth != lpInfo->Month || time.wYear != lpInfo->Year)
	{
		CloseHandle(lpInfo->File);

		lpInfo->Day = time.wDay;

		lpInfo->Month = time.wMonth;

		lpInfo->Year = time.wYear;

		wsprintf(lpInfo->Filename,".\\%s\\%04d-%02d-%02d.log",lpInfo->Directory,lpInfo->Year,lpInfo->Month,lpInfo->Day);

		lpInfo->File = CreateFile(lpInfo->Filename,GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);

		if(lpInfo->File == INVALID_HANDLE_VALUE)
		{
			lpInfo->Active = 0;
			return;
		}

		if(SetFilePointer(lpInfo->File,0,0,FILE_END) == INVALID_SET_FILE_POINTER)
		{
			lpInfo->Active = 0;
			CloseHandle(lpInfo->File);
			return;
		}
	}

	char temp[1024] = {0};

	va_list arg;
	va_start(arg,text);
	vsprintf_s(temp,text,arg);
	va_end(arg);

	char buff[1024] = {0};

	wsprintf(buff,"%02d:%02d:%02d %s\r\n",time.wHour,time.wMinute,time.wSecond,temp);

	#if(ENCRYPT_LOG==1)

	for(int n=0;n < (int)strlen(buff);n++)
	{
		buff[n] ^= 0xE1;
		buff[n] += 0x12;
	}

	#endif

	DWORD OutSize;

	WriteFile(lpInfo->File,buff,strlen(buff),&OutSize,0);
}

inline std::string getCurrentDateTime(std::string s) {
	time_t now = time(0);
	struct tm  tstruct;
	char  buf[80];
	tstruct = *localtime(&now);
	if (s == "now")
		strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
	else if (s == "date")
		strftime(buf, sizeof(buf), "%Y-%m-%d", &tstruct);
	return  std::string(buf);
};



void CLog::Log(char* logMsg) {
	std::string filePath = "./log_" + getCurrentDateTime("date") + ".txt";
	std::string now = getCurrentDateTime("now");
	std::ofstream ofs(filePath.c_str(), std::ios_base::out | std::ios_base::app);
	ofs << now << '\t' << logMsg << '\n';
	ofs.close();
}
