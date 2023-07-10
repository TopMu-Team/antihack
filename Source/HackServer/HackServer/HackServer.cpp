#include "stdafx.h"
#include "resource.h"
#include "HackServer.h"
#include "MiniDump.h"
#include "ProcessManager.h"
#include "ServerDisplayer.h"
#include "ServerInfo.h"
#include "SocketManager.h"
#include "ReadFiles.h"
#include "BlackList.h"
#include "Authenticate.h"
#include "Util.h"
#include "ClientManager.h"
HINSTANCE hInst;
TCHAR szTitle[MAX_LOADSTRING];
TCHAR szWindowClass[MAX_LOADSTRING];
HWND hWnd;

int APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow) // OK
{
	try {
		CMiniDump::Start();

		LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
		LoadString(hInstance, IDC_HACKSERVER, szWindowClass, MAX_LOADSTRING);

		MyRegisterClass(hInstance);

		if (InitInstance(hInstance, nCmdShow) == 0)
		{
			return 0;
		}

		gServerInfo.ReadStartupInfo("AntiHackServerInfo", ".\\AntiHackServer.ini");

#if(PROTECT_STATE == 1)

		gAuthenticate.Check();

#endif

		char buff[256];

		wsprintf(buff, "[%s] TopMu AntiHackServer (UserCount : %d)", HACKSERVER_VERSION, 0);

		SetWindowText(hWnd, buff);

		gServerDisplayer.Init(hWnd);

		WSADATA wsa;

		if (WSAStartup(MAKEWORD(2, 2), &wsa) == 0)
		{
			if (gSocketManager.Start(gServerInfo.m_HackServerPort) != 0)
			{
				gServerInfo.ReadInit();

				SetTimer(hWnd, TIMER_1000, 1000, 0);

				SetTimer(hWnd, TIMER_5000, 5000, 0);
			}
		}
		else
		{
			LogAdd(LOG_RED, "WSAStartup() failed with error: %d", WSAGetLastError());
		}

		gServerDisplayer.PaintAllInfo();

		SetTimer(hWnd, TIMER_2000, 2000, 0);

		HACCEL hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_HACKSERVER);

		MSG msg;

		while (GetMessage(&msg, 0, 0, 0) != 0)
		{
			if (TranslateAccelerator(msg.hwnd, hAccelTable, &msg) == 0)
			{
				TranslateMessage(&msg);
				DispatchMessageA(&msg);
			}
		}

		CMiniDump::Clean();

		return msg.wParam;
	} catch(...) {}
}

ATOM MyRegisterClass(HINSTANCE hInstance) // OK
{
	try {
		WNDCLASSEX wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = (WNDPROC)WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_HACKSERVER);
		wcex.hCursor = LoadCursor(0, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = (LPCSTR)IDC_HACKSERVER;
		wcex.lpszClassName = szWindowClass;
		wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

		return RegisterClassEx(&wcex);
	} catch(...) {}
}

BOOL InitInstance(HINSTANCE hInstance,int nCmdShow) // OK
{
	hInst = hInstance;

	hWnd = CreateWindow(szWindowClass,szTitle,WS_OVERLAPPEDWINDOW | WS_THICKFRAME,CW_USEDEFAULT,0,668,600,0,0,hInstance,0);

	if(hWnd == 0)
	{
		return 0;
	}

	ShowWindow(hWnd,nCmdShow);
	UpdateWindow(hWnd);
	return 1;
}

bool CheckListBox(HWND hDlg, int Item, char* text)
{
	try {
		HWND hlst;
		SetClassLong(hDlg, GCL_HICON, (long)LoadIcon(0, IDI_APPLICATION));
		hlst = GetDlgItem(hDlg, Item);

		int count = SendMessage(hlst, LB_GETCOUNT, 0, 0);

		char buff[100];

		for (int i = 0; i < count; i++)
		{
			SendMessage(hlst, LB_GETTEXT, (WPARAM)i, (LPARAM)buff);
			//_CRT_SECURE_NO_WARNINGS
			const char* SEPARATORS = " ";
			char* ptr = 0;       //Указател?		ptr = strtok(buff,SEPARATORS); //Выдираем первое слов?из строки
			ptr = strtok(0, SEPARATORS);   //Подбирае?слов?
			if (!strcmp(ptr, text))
			{
				return false;
			}
			ptr = 0;
		}

		return true;
	} catch(...) {}
}

bool GetSelecterListBoxItem(HWND hDlg, int Item, char* text)
{
	try {
		//char text[64]={0};
		HWND hlst;
		SetClassLong(hDlg, GCL_HICON, (long)LoadIcon(0, IDI_APPLICATION));
		hlst = GetDlgItem(hDlg, Item);

		int count = SendMessage(hlst, LB_GETCOUNT, 0, 0);
		int iSelected = -1;

		for (int i = 0; i < count; i++)
		{
			if (SendMessage(hlst, LB_GETSEL, i, 0) > 0)
			{
				iSelected = i;
				break;
			}
		}

		if (iSelected != -1)
		{
			SendMessage(hlst, LB_GETTEXT, (WPARAM)iSelected, (LPARAM)text);

			return 1;

			//LogAdd(LOG_RED,"Selected = %s",text);

			/*for(int n=0;n < MAX_CLIENT;n++)
			{
			if(gClientManager[n].CheckState() != 0)
			{
			count++;

			sprintf(text,"%d). %s",count,gClientManager[n].m_IpAddr);
			SendMessage(hlst,LB_ADDSTRING,NULL,LPARAM(text));
			}
			}*/
		}

		return 0;
	} catch(...){}
}



int GetIndexByIpAndHwid(char* IP, char* HWID)
{
	try {
		for (int n = 0; n < MAX_CLIENT; n++)
		{
			if (gClientManager[n].CheckState() != 0)
			{
				if (!strcmp(gClientManager[n].m_IpAddr, IP) && !strcmp(gClientManager[n].m_HardwareId, HWID))
				{
					return gClientManager[n].m_index;
				}
			}
		}

		return -1;
	} catch(...){ }
}



LRESULT CALLBACK PlayersList(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) // OK
{

	try {
		switch (message)
		{
		case WM_INITDIALOG:
		{
			SetClassLong(hDlg, GCL_HICON, (long)LoadIcon(0, IDI_APPLICATION));
			char WindowName[100];
			sprintf(WindowName, "Connected %d Users", GetUserCount());
			SetWindowText(hDlg, WindowName);
			HWND IpListBox = GetDlgItem(hDlg, IDC_LIST_IP);


			int i = 0;

			while (gClientManager[i].CheckState() != 0)
			{
				char text[64];
				sprintf(text, "%d). %s", i, gClientManager[i].m_IpAddr);
				SendMessage(IpListBox, LB_ADDSTRING, NULL, LPARAM(text));
				i++;
			}
		}
		break;

		case WM_COMMAND:
			int wmId = LOWORD(wParam);
			int wmEvent = HIWORD(wParam);

			switch (wmId)
			{
			case IDC_LIST_IP:
			{

				if (HIWORD(wParam) == LBN_SELCHANGE)
				{
					HWND IpListBox = GetDlgItem(hDlg, IDC_LIST_IP);
					HWND HwidListBox = GetDlgItem(hDlg, IDC_LIST_HWID);

					SendMessage(HwidListBox, LB_RESETCONTENT, 0, NULL);

					char Buf[100];
					int ListItem1 = (int)SendMessage(IpListBox, LB_GETCURSEL, 0, 0);
					if (ListItem1 != LB_ERR)
					{
						char text[64];
						sprintf(text, "%s", gClientManager[ListItem1].m_HardwareId);
						SendMessage(HwidListBox, LB_ADDSTRING, NULL, LPARAM(text));
					}
					return 0;
				}

			}
			break;
			case ID_CANCEL: 
			{
				try {
					HWND IpListBox = GetDlgItem(hDlg, IDC_LIST_IP);
					HWND HwidListBox = GetDlgItem(hDlg, IDC_LIST_HWID);
					int ListItem1 = (int)SendMessage(IpListBox, LB_GETCURSEL, 0, 0);
					gClientManager[ListItem1].DelClient();
				} catch(...){}
				break;
			}
			case ID_RELOAD:
			{

				SetClassLong(hDlg, GCL_HICON, (long)LoadIcon(0, IDI_APPLICATION));
				HWND hlst = GetDlgItem(hDlg, IDC_LIST_IP);
				HWND HwidListBox = GetDlgItem(hDlg, IDC_LIST_HWID);
				SendMessage(hlst, LB_RESETCONTENT, 0, NULL);
				SendMessage(HwidListBox, LB_RESETCONTENT, 0, NULL);

				int count = 0;
				char text[64];

				for (int n = 0; n < MAX_CLIENT; n++)
				{
					if (gClientManager[n].CheckState() != 0)
					{
						if (CheckListBox(hDlg, IDC_LIST_IP, gClientManager[n].m_IpAddr) == 0)
						{
							continue;
						}
						count++;
						sprintf(text, "%d). %s", count, gClientManager[n].m_IpAddr);
						SendMessage(hlst, LB_ADDSTRING, NULL, LPARAM(text));
					}
				}
			}
			break;		
			case ID_DESKTOP:
			{
				HWND IpListBox = GetDlgItem(hDlg, IDC_LIST_IP);
				HWND HwidListBox = GetDlgItem(hDlg, IDC_LIST_HWID);
				int ListItem1 = (int)SendMessage(IpListBox, LB_GETCURSEL, 0, 0);
				if (ListItem1 != LB_ERR)
				{

					std::set<std::string> strings = gClientManager[ListItem1].process;

					for (std::string s: strings) {
						char text[64];
						strcpy(text, s.c_str());
						SendMessage(HwidListBox, LB_ADDSTRING, NULL, LPARAM(text));
					}
				}
				break;
			}
			default:
				EndDialog(hDlg, LOWORD(wParam));
				break;
			}
		}

		return 0;
	}
	catch (...) {

	}
}

LRESULT CALLBACK WndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) // OK
{
	try {
		switch (message)
		{
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDM_ABOUT:
				DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
				break;
			case IDM_EXIT:
				if (MessageBox(0, "Are you sure to terminate AntiHackServer?", "Ask terminate server", MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					DestroyWindow(hWnd);
				}
				break;
			case ID_OTHER_SCREEN:
				DialogBox(hInst, (LPCTSTR)IDD_INFO_DIALOG, hWnd, (DLGPROC)PlayersList);
				break;
			case IDM_RELOAD_CONFIG:
				gServerInfo.ReadConfig();
				break;
			case IDM_RELOAD_DUMP:
				gServerInfo.ReadDumpList();
				break;
			case IDM_RELOAD_CHECKSUM:
				gServerInfo.ReadChecksumList();
				break;
			case IDM_RELOAD_WINDOW:
				gServerInfo.ReadWindowList();
				break;
			case IDM_RELOAD_BLACKLIST:
				gServerInfo.ReadBlackList();
				break;
			case IDM_UPDATE:
				gReadFiles.UpdateInternalList();
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		case WM_TIMER:
			switch (wParam)
			{
			case TIMER_1000:
				gProcessManager.CheckProcess();
				break;
			case TIMER_2000:
				gServerDisplayer.Run();
				break;
			case TIMER_5000:
				TimeoutProc();
				gProcessManager.ClearProcessCache();
				break;
			default:
				break;
			}
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}

		return 0;
	} catch(...) {}
}

LRESULT CALLBACK About(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam) // OK
{
	switch(message)
	{
		case WM_INITDIALOG:
			return 1;
		case WM_COMMAND:
			if(LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg,LOWORD(wParam));
				return 1;
			}
			break;
	}

	return 0;
}
