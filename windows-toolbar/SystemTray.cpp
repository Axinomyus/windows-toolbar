#include "SystemTray.h"
#include "Globals.h"
#include "Constants.h"
#include "resource.h"
#include <shellapi.h>

void AddTrayIcon(HWND hwnd) 
{
	NOTIFYICONDATAW nid		= { 0 };
	nid.cbSize				= sizeof(NOTIFYICONDATAW);
	nid.hWnd				= hwnd;
	nid.uID					= TRAY_ICON_ID;
	nid.uFlags				= NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage	= WM_TRAYICON;

	HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAINICON));
	if (!hIcon) 
	{
		hIcon = LoadIcon(GetModuleHandle(NULL), IDI_APPLICATION);
	}

	nid.hIcon = hIcon;
	wcscpy_s(nid.szTip, 128, L"PowerToolbar");

	Shell_NotifyIconW(NIM_ADD, &nid);
	g_trayIconAdded = true;
}

void RemoveTrayIcon() 
{
	if (!g_trayIconAdded)
	{
		return;
	}

	NOTIFYICONDATAW nid = { 0 };
	nid.cbSize			= sizeof(NOTIFYICONDATAW);
	nid.uID				= TRAY_ICON_ID;
	nid.hWnd			= g_toolbar;

	Shell_NotifyIconW(NIM_DELETE, &nid);
	g_trayIconAdded = false;
}

void ShowTrayContextMenu(HWND hwnd, POINT pt) 
{
	HMENU hMenu = CreatePopupMenu();
	AppendMenuW(hMenu, MF_STRING, 1000, L"Open");
	AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenuW(hMenu, MF_STRING, 1001, L"Exit");

	SetForegroundWindow(hwnd);
	TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
	DestroyMenu(hMenu);
}