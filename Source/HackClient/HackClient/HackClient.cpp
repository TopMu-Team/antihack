#include "stdafx.h"
#include "HackClient.h"
#include "Connection.h"
#include "FileProtect.h"
#include "HackCheck.h"
#include "HackServerProtocol.h"
#include "ListManager.h"
#include "Log.h"
#include "Message.h"
#include "ProcessManager.h"
#include "Protect.h"
#include "SplashScreen.h"
#include "resource.h"
#include "Util.h"
//Modules
#include "DumpCheck.h"
#include "ExecutableCheck.h"
#include "FileCheck.h"
#include "FileMappingCheck.h"
#include "LibraryCheck.h"
#include "MacroCheck.h"
#include "ProcessCheck.h"
#include "RegistryCheck.h"
#include "SimpleModules.h"
#include "ThreadCheck.h"
#include "WindowCheck.h"
#include <ctime>
#include "ScreenCapture.h"
#include "d3d9.h"
#include <string> // for string and to_string()
#include <iostream>
#include <windows.h>
#include <TlHelp32.h>
#include "../RebirthGuard/RGString.h";
#pragma comment (lib, "RebirthGuard.lib")



using namespace std;


void Registry()
{
	HKEY hKey;
	LPCTSTR sk = TEXT("Software\\Webzen\\Mu\\Config");
	LONG openRes = RegOpenKeyEx(HKEY_CURRENT_USER, sk, 0, KEY_ALL_ACCESS, &hKey);
	if (openRes == ERROR_SUCCESS) {
		LPCTSTR value = TEXT("AhData");
		LPCTSTR data = "OK\0";
		LONG setRes = RegSetValueEx(hKey, value, 0, REG_SZ, (LPBYTE)data, strlen(data) + 1);
		if (setRes == ERROR_SUCCESS) {
			//printf("Success writing to Registry.");
		}
		else {
			//printf("Error writing to Registry.");
		}
	}
	else {
		// printf("Error opening key.");
	}
	LONG closeOut = RegCloseKey(hKey);
	if (closeOut == ERROR_SUCCESS) {
		//printf("Success closing key.");
	}
	else {
		//printf("Error closing key.");
	}
	//return 1;
}

HINSTANCE hins;
HANDLE ThreadHandles[3];

DWORD WINAPI ConnectionReconnectThread() // OK
{
	while (!DelayMe(5000, 100))
	{
		//Registry();
		if (gReconnectStatus == 1)
		{
			if (gConnection.Init(HackServerProtocolCore) == 0)
			{
				SplashScreen(&SplashError, 2, 1, gMessage.GetMessage(1), 5000);
				continue;
			}

			if (gConnection.Connect(gProtect.m_MainInfo.IpAddress, gProtect.m_MainInfo.ServerPort) == 0)
			{
				gConnectionStatusTime = GetTickCount();
				continue;
			}

			CHClientInfoSend();

			gReconnectStatus = 2;

			gConnectionStatusTime = GetTickCount();
		}
	}

	return 0;
}

DWORD WINAPI ConnectionStatusThread() // OK
{
	while (!DelayMe(5000, 100))
	{
		//Registry();
		if (gConnection.CheckState() == 0)
		{
			if (gReconnectSwitch == 0)
			{
				SplashScreen(&SplashError, 2, 1, gMessage.GetMessage(2), 5000);
				continue;
			}

			if (gReconnectStatus == 1)
			{
				gConnectionStatusTime = GetTickCount();
				continue;
			}

			if (gReconnectStatus == 0 || gReconnectStatus == 2)
			{
				gReconnectStatus = 1;
				continue;
			}
		}
		else
		{
			if ((GetTickCount() - gConnectionStatusTime) > 60000)
			{
				SplashScreen(&SplashError, 2, 1, gMessage.GetMessage(3), 5000);
				continue;
			}
			else
			{
				CHConnectionStatusSend();
				continue;
			}
		}
	}

	return 0;
}

void convertCharPtrToCharArray(const char* charPtr, char charArray[64]) {
	strncpy(charArray, charPtr, 63);  // Copy up to 63 characters from charPtr to charArray
	charArray[63] = '\0';  // Null-terminate the charArray
}

DWORD WINAPI ScreenThread() // OK
{
	DWORD CycleExecutionDelay = 500;


	while (!DelayMe(1000, 10))
	{

		try {
			int count;

			char* concatenatedNames = GetProcess(count);
			char* token = strtok(concatenatedNames, ",");
			while (token != NULL) {
				SHDP_SHOW_PROCESS_SEND pMsg;
				pMsg.header.set(0x04, sizeof(pMsg));
				strncpy(pMsg.process, token, 63);
				gConnection.DataSend((BYTE*)&pMsg, pMsg.header.size);
				token = strtok(NULL, ",");
			}
		}
		catch (...) {}
	}
	return 0;
}

DWORD WINAPI MainThread() // OK
{

	//Registry();//

	DWORD CycleCount = 0;

	DWORD CycleExecutionDelay = 500;

	LARGE_INTEGER Frequency;

	LARGE_INTEGER InitCounter;

	LARGE_INTEGER NextCounter;

	LARGE_INTEGER ElapsedMicroseconds;

	QueryPerformanceFrequency(&Frequency);

	QueryPerformanceCounter(&InitCounter);

	while (!DelayMe(((CycleExecutionDelay > 500) ? 0 : (500 - CycleExecutionDelay)), 1))
	{
		Registry();
		QueryPerformanceCounter(&NextCounter);

		ElapsedMicroseconds.QuadPart = ((NextCounter.QuadPart - InitCounter.QuadPart) * 1000000) / Frequency.QuadPart;

		if ((ElapsedMicroseconds.QuadPart / 1000) > 1500)
		{
			SplashScreen(&SplashError, 2, 1, gMessage.GetMessage(0), 5000);
			return 0;
		}

		QueryPerformanceFrequency(&Frequency);

		QueryPerformanceCounter(&InitCounter);

		if (gDetectCloseTime != 0)
		{
			if ((GetTickCount() - gDetectCloseTime) > 10000)
			{
				SplashScreen(&SplashError, 2, 1, gMessage.GetMessage(21), 5000);
				return 0;
			}
		}

		if (gIpAddressAddress != 0)
		{
			MemoryCpy(gIpAddressAddress, gIpAddress, sizeof(gIpAddress));
		}

		if (gClientVersionAddress != 0)
		{
			MemoryCpy(gClientVersionAddress, gClientVersion, sizeof(gClientVersion));
		}

		if (gClientSerialAddress != 0)
		{
			MemoryCpy(gClientSerialAddress, gClientSerial, sizeof(gClientSerial));
		}

		switch (((CycleCount++) % 10))
		{
		case 0:
			API_SCAN();
			gWindowCheck.Scan();
			gProcessManager.CheckProcess(15);
			break;
		case 1:
			CheckDetourIntegrity();
			MEMORY_PROTECTION_SCAN();
			gRegistryCheck.Scan();
			break;
		case 2:
			API_SCAN();
			gWindowCheck.Scan();
			gProcessManager.CheckProcess(15);
			break;
		case 3:
			DEBUGGER_SCAN();
			MEMORY_PROTECTION_SCAN();
			gFileCheck.Scan();
			break;
		case 4:
			API_SCAN();
			gWindowCheck.Scan();
			gProcessManager.CheckProcess(15);
			break;
		case 5:
			CheckDetourIntegrity();
			MEMORY_PROTECTION_SCAN();
			HANDLE_PROTECTION_SCAN();
			break;
		case 6:
			API_SCAN();
			gWindowCheck.Scan();
			gProcessManager.CheckProcess(15);
			break;
		case 7:
			DEBUGGER_SCAN();
			MEMORY_PROTECTION_SCAN();
			gFileMappingCheck.Scan();
			break;
		case 8:
			API_SCAN();
			gWindowCheck.Scan();
			gProcessManager.CheckProcess(15);
			break;
		case 9:
			CheckDetourIntegrity();
			MEMORY_PROTECTION_SCAN();
			gProcessManager.ClearProcessCache();
			break;
		default:
			break;
		}

		QueryPerformanceCounter(&NextCounter);

		ElapsedMicroseconds.QuadPart = ((NextCounter.QuadPart - InitCounter.QuadPart) * 1000000) / Frequency.QuadPart;

		CycleExecutionDelay = (DWORD)(ElapsedMicroseconds.QuadPart / 1000);

		QueryPerformanceFrequency(&Frequency);

		QueryPerformanceCounter(&InitCounter);


	}

	return 0;
}
//
//extern "C" _declspec(dllexport) void EntryProc() // OK
//{
//	gLog.AddLog(1,"Log");
//
//	CheckSystemInformation();
//
//	if(gProtect.ReadMainFile("igk.info") == 0)
//	{
//		gLog.Output(LOG_DEBUG,GetEncryptedString(18));
//		SafeExitProcess();
//		return;
//	}
//
//	if(LIBRARY_LOAD_DETACH() == 0)
//	{
//		gLog.Output(LOG_DEBUG,GetEncryptedString(19));
//		SafeExitProcess();
//		return;
//	}
//
//	if(MEMORY_CHECK_DETACH() == 0)
//	{
//		gLog.Output(LOG_DEBUG,GetEncryptedString(20));
//		SafeExitProcess();
//		return;
//	}
//
//	if(SetAdminPrivilege(SE_DEBUG_NAME) == 0)
//	{
//		gLog.Output(LOG_DEBUG,GetEncryptedString(21));
//		SafeExitProcess();
//		return;
//	}
//
//	if(API_INIT() == 0)
//	{
//		gLog.Output(LOG_DEBUG,GetEncryptedString(22));
//		SafeExitProcess();
//		return;
//	}
//
//	if(gProcessManager.Init() == 0)
//	{
//		gLog.Output(LOG_DEBUG,GetEncryptedString(23));
//		SafeExitProcess();
//		return;
//	}
//
//	if(MEMORY_PROTECTION_INIT() == 0)
//	{
//		gLog.Output(LOG_DEBUG,GetEncryptedString(45));
//		SafeExitProcess();
//		return;
//	}
//
//	if(HANDLE_PROTECTION_INIT() == 0)
//	{
//		gLog.Output(LOG_DEBUG,GetEncryptedString(46));
//		SafeExitProcess();
//		return;
//	}
//
//	if(gConnection.Init(HackServerProtocolCore) == 0)
//	{
//		SplashScreen(&SplashError,2,1,gMessage.GetMessage(4),5000);
//		return;
//	}
//
//	char buff[256];
//
//	wsprintf(buff,gMessage.GetMessage(5),gProtect.m_MainInfo.ServerName);
//
//	SplashScreen(&SplashInit,0,1,buff,0);
//
//	if(gConnection.Connect(gProtect.m_MainInfo.IpAddress,gProtect.m_MainInfo.ServerPort) == 0)
//	{
//		SplashScreen(&SplashError,2,1,gMessage.GetMessage(6),5000);
//		return;
//	}
//
//	CHClientInfoSend();
//
//	DWORD ClientInfoTimeOut = GetTickCount();
//
//	while(!DelayMe(500,1))
//	{
//		if((GetTickCount()-ClientInfoTimeOut) > 60000)
//		{
//			SplashScreen(&SplashError,2,1,gMessage.GetMessage(7),5000);
//			return;
//		}
//
//		if(gConnection.CheckState() == 0 && gConnection.Init(HackServerProtocolCore) != 0)
//		{
//			if(gConnection.Connect(gProtect.m_MainInfo.IpAddress,gProtect.m_MainInfo.ServerPort) != 0)
//			{
//				CHClientInfoSend();
//				continue;
//			}
//		}
//
//		if(gClientInfoOK != 0 && gDumpListOK != 0 && gChecksumListOK != 0 && gInternalListOK != 0 && gWindowListOK != 0)
//		{
//			DWORD CurProgress = gListManager.gDumpListInfo.size()+gListManager.gChecksumListInfo.size()+gListManager.gInternalListInfo.size()+gListManager.gWindowListInfo.size();
//
//			DWORD MaxProgress = gDumpListMaxCount+gChecksumListMaxCount+gInternalListMaxCount+gWindowListMaxCount;
//
//			if(CurProgress == MaxProgress)
//			{
//				break;
//			}
//		}
//	}
//
//	gLog.Output(LOG_DEBUG,GetEncryptedString(24),gDumpListMaxCount,gChecksumListMaxCount,gInternalListMaxCount,gWindowListMaxCount);
//
//	if(MEMORY_CHECK_DETACH() == 0)
//	{
//		SplashScreen(&SplashError,2,1,gMessage.GetMessage(20),5000);
//		return;
//	}
//
//	InitHackCheck();
//
//	gProtect.CheckClientFile();
//
//	gProtect.CheckVerifyFile();
//
//	ThreadHandles[0] = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)MainThread,0,0,(DWORD*)&gThreadCheck.m_CheckThreadID[1]);
//
//	ThreadHandles[1] = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ConnectionStatusThread,0,0,(DWORD*)&gThreadCheck.m_CheckThreadID[2]);
//
//	ThreadHandles[2] = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ConnectionReconnectThread,0,0,(DWORD*)&gThreadCheck.m_CheckThreadID[3]);
//
//	ThreadHandles[3] = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ScreenThread,0,0,(DWORD*)&gThreadCheck.m_CheckThreadID[5]);
//
//	SetThreadPriority(ThreadHandles[0],THREAD_PRIORITY_HIGHEST);
//
//	WaitForMultipleObjects(4,ThreadHandles,1,2000);
//
//	gMacroCheck.Init(hins);
//
//	gThreadCheck.Init();
//
//	gFileProtect.Init();
//
//	SplashInit.CloseSplash();
//
//	gProtect.CheckPluginFile();
//
//	ThanhBinh();
//
//	gLog.Output(LOG_DEBUG,GetEncryptedString(44));
//}
//



BOOL WINAPI ATTACH() {
	CheckSystemInformation();
	if(gProtect.ReadMainFile("Antihack.info") == 0)
	{
		gLog.Output(LOG_DEBUG,GetEncryptedString(18));
		SafeExitProcess();
		return 0;
	}

	/*if (MEMORY_CHECK_DETACH() == 0)
	{
		gLog.Output(LOG_DEBUG, GetEncryptedString(20));
		SafeExitProcess();
		return 0;
	}*/

	if (SetAdminPrivilege(SE_DEBUG_NAME) == 0)
	{
		gLog.Output(LOG_DEBUG, GetEncryptedString(21));
		SafeExitProcess();
		return 0;
	}

	if (API_INIT() == 0)
	{
		gLog.Output(LOG_DEBUG, GetEncryptedString(22));
		SafeExitProcess();
		return 0;
	}

	if (gProcessManager.Init() == 0)
	{
		gLog.Output(LOG_DEBUG, GetEncryptedString(23));
		SafeExitProcess();
		return 0;
	}

	if (MEMORY_PROTECTION_INIT() == 0)
	{
		gLog.Output(LOG_DEBUG, GetEncryptedString(45));
		SafeExitProcess();
		return 0;
	}

	if (HANDLE_PROTECTION_INIT() == 0)
	{
		gLog.Output(LOG_DEBUG, GetEncryptedString(46));
		SafeExitProcess();
		return 0;
	}


	if (gConnection.Init(HackServerProtocolCore) == 0)
	{
		SplashScreen(&SplashError, 2, 1, gMessage.GetMessage(4), 5000);
		return 0;
	}

	if (gConnection.Connect("15.235.181.48", 55111) == 0)
	{
		SplashScreen(&SplashError, 2, 1, gMessage.GetMessage(6), 5000);
		return 0;
	}
	CHClientInfoSend();
	DWORD ClientInfoTimeOut = GetTickCount();

	while(!DelayMe(500,1))
	{
		if((GetTickCount()-ClientInfoTimeOut) > 60000)
		{
			SplashScreen(&SplashError,2,1,gMessage.GetMessage(7),5000);
			return 0;
		}

		if(gConnection.CheckState() == 0 && gConnection.Init(HackServerProtocolCore) != 0)
		{
			if(gConnection.Connect("15.235.181.48", 55111) != 0)
			{
				CHClientInfoSend();
				continue;
			}
		}

		if(gClientInfoOK != 0 && gDumpListOK != 0 && gChecksumListOK != 0 && gInternalListOK != 0 && gWindowListOK != 0)
		{
			DWORD CurProgress = gListManager.gDumpListInfo.size()+gListManager.gChecksumListInfo.size()+gListManager.gInternalListInfo.size()+gListManager.gWindowListInfo.size();

			DWORD MaxProgress = gDumpListMaxCount+gChecksumListMaxCount+gInternalListMaxCount+gWindowListMaxCount;

			if(CurProgress == MaxProgress)
			{
				break;
			}
		}
	}


	
	InitHackCheck();
	
	
	//ThreadHandles[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MainThread, 0, 0, (DWORD*)&gThreadCheck.m_CheckThreadID[1]);

	ThreadHandles[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ConnectionStatusThread, 0, 0, (DWORD*)&gThreadCheck.m_CheckThreadID[2]);

	ThreadHandles[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ConnectionReconnectThread, 0, 0, (DWORD*)&gThreadCheck.m_CheckThreadID[2]);
	ThreadHandles[2] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ScreenThread, 0, 0, (DWORD*)&gThreadCheck.m_CheckThreadID[3]);
	
	
	SetThreadPriority(ThreadHandles[0],THREAD_PRIORITY_HIGHEST);
	
	WaitForMultipleObjects(4,ThreadHandles,1,2000);
	
	gMacroCheck.Init(hins);
	
	gThreadCheck.Init();
	
	gFileProtect.Init();
	
	SplashInit.CloseSplash();
	
	
	SplashScreen(&SplashInit, 0, 1, "TopMu", 2000);
	SplashInit.CloseSplash();
	return 1;
}

#ifdef _WIN64
// 64-bit application
#define ReadPEB() reinterpret_cast<PPEB>(__readgsqword(0x60))
#else
// 32-bit application
#ifdef _MSC_VER
#define ReadPEB() reinterpret_cast<PPEB>(__readfsdword(0x30))
#else
#ifdef __GNUC__
#define ReadPEB() reinterpret_cast<PPEB>(__readfsdword(0x18))
#endif
#endif
#endif

BOOL APIENTRY DllMain(HANDLE hModule,DWORD ul_reason_for_call,LPVOID lpReserved) // OK
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		hins = (HINSTANCE)hModule;
		DisableThreadLibraryCalls(hins);
		RGS("Hello RebirthGuard SampleDLL!\n");
		CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(ATTACH), hins, 0, nullptr);
		DWORD parentProcessId = 0;
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot != INVALID_HANDLE_VALUE)
		{
			PROCESSENTRY32 pe32 = { sizeof(PROCESSENTRY32) };
			if (Process32First(hSnapshot, &pe32))
			{
				do
				{
					if (pe32.th32ProcessID == GetCurrentProcessId())
					{
						parentProcessId = pe32.th32ParentProcessID;
						break;
					}
				} while (Process32Next(hSnapshot, &pe32));
			}
			CloseHandle(hSnapshot);
		}

		bool isParentProcessTrusted = false;
		HANDLE hParentProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, parentProcessId);
		if (hParentProcess != NULL)
		{
			
			isParentProcessTrusted = true; 
			CloseHandle(hParentProcess);
		}

		if (!isParentProcessTrusted)
		{
			ExitProcess(0);
		}

		
	}

	return 1;
}



__declspec(dllexport) std::size_t EncryptFunctionName(const std::string& str) {
	std::hash<std::string> hasher;
	return hasher(str);
}