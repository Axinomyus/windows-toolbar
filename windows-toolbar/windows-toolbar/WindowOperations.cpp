#include "WindowOperations.h"
#include "Globals.h"
#include <tlhelp32.h>
#include <algorithm>

std::wstring GetProcessNameFromHWND(HWND hwnd) 
{
	DWORD pid;
	GetWindowThreadProcessId(hwnd, &pid);
	HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (!hProc) 
	{
		return L"";
	}

	wchar_t name[MAX_PATH] = {};
	DWORD size = MAX_PATH;
	if (QueryFullProcessImageNameW(hProc, 0, name, &size)) 
	{
		CloseHandle(hProc);
		std::wstring path(name);
		size_t pos	= path.find_last_of(L"\\/");
		return (pos != std::wstring::npos) ? path.substr(pos + 1) : path;
	}

	CloseHandle(hProc);
	return L"";
}

void CloseActiveWindow(HWND hwnd) 
{
	if (!hwnd || hwnd == g_toolbar || hwnd == g_minimizedToolbar)
	{
		return;
	}

	SendMessage(hwnd, WM_CLOSE, 0, 0);
}

void CloseActiveProcess(HWND hwnd) 
{
	if (!hwnd || hwnd == g_toolbar)
	{
		return;
	}

	DWORD pid;
	GetWindowThreadProcessId(hwnd, &pid);
	HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
	if (h) 
	{
		TerminateProcess(h, 0);
		CloseHandle(h);
	}
}

void ToggleFullscreen(HWND hwnd) 
{
	if (!hwnd || hwnd == g_toolbar)
	{
		return;
	}

	static RECT saved = {};
	LONG style = GetWindowLong(hwnd, GWL_STYLE);

	if (style & WS_OVERLAPPEDWINDOW) 
	{
		GetWindowRect(hwnd, &saved);
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);
		SetWindowLong(hwnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
		SetWindowPos(hwnd, HWND_TOP,
			mi.rcMonitor.left, mi.rcMonitor.top,
			mi.rcMonitor.right - mi.rcMonitor.left,
			mi.rcMonitor.bottom - mi.rcMonitor.top,
			SWP_FRAMECHANGED | SWP_NOZORDER);
	}
	else 
	{
		SetWindowLong(hwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
		SetWindowPos(hwnd, HWND_NOTOPMOST,
			saved.left, saved.top,
			saved.right - saved.left,
			saved.bottom - saved.top,
			SWP_FRAMECHANGED | SWP_NOZORDER);
	}
}

void ToggleAlwaysOnTop(HWND hwnd) 
{
	if (!hwnd || hwnd == g_toolbar)
	{
		return;
	}

	BOOL topmost = (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST);
	SetWindowPos(hwnd, topmost ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void MinimizeWindow(HWND hwnd) 
{
	if (!hwnd || hwnd == g_toolbar)
	{
		return;
	}

	ShowWindow(hwnd, SW_MINIMIZE);
}

void CenterWindow(HWND hwnd) 
{
	if (!hwnd || hwnd == g_toolbar)
	{
		return;
	}

	RECT rc;
	GetWindowRect(hwnd, &rc);
	int w = rc.right - rc.left;
	int h = rc.bottom - rc.top;

	MONITORINFO mi = { sizeof(mi) };
	GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);

	int x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - w) / 2;
	int y = mi.rcWork.top + (mi.rcWork.bottom - mi.rcWork.top - h) / 2;
	SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void KillAllByProcessName(HWND hwnd) 
{
	if (!hwnd || hwnd == g_toolbar)
	{
		return;
	}

	std::wstring target = GetProcessNameFromHWND(hwnd);
	if (target.empty())
	{
		return;
	}

	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snap == INVALID_HANDLE_VALUE)
	{
		return;
	}

	PROCESSENTRY32W pe = { sizeof(pe) };
	if (Process32FirstW(snap, &pe)) 
	{
		do 
		{
			if (_wcsicmp(pe.szExeFile, target.c_str()) == 0) 
			{
				HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
				if (h) 
				{
					TerminateProcess(h, 0);
					CloseHandle(h);
				}
			}
		} while (Process32NextW(snap, &pe));
	}
	CloseHandle(snap);
}

bool IsSystemUIWindow(HWND hwnd) 
{
	if (!hwnd) return true;

	wchar_t className[256] = { 0 };
	GetClassNameW(hwnd, className, 256);

	if (wcscmp(className, L"Shell_TrayWnd")						== 0) return true;
	if (wcscmp(className, L"Shell_SecondaryTrayWnd")			== 0) return true;
	if (wcscmp(className, L"Windows.UI.Core.CoreWindow")		== 0) return true;
	if (wcscmp(className, L"Start")								== 0) return true;
	if (wcscmp(className, L"DV2ControlHost")					== 0) return true;
	if (wcscmp(className, L"ImmersiveLauncher")					== 0) return true;
	if (wcscmp(className, L"Shell_InputSwitchTopLevelWindow")	== 0) return true;
	if (wcscmp(className, L"MultitaskingViewFrame")				== 0) return true;

	if (wcscmp(className, L"Button") == 0) 
	{
		HWND parent = GetParent(hwnd);
		if (parent) 
		{
			wchar_t parentClass[256] = { 0 };
			GetClassNameW(parent, parentClass, 256);
			if (wcscmp(parentClass, L"Shell_TrayWnd") == 0) return true;
		}
	}

	DWORD pid;
	GetWindowThreadProcessId(hwnd, &pid);
	HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (hProc) 
	{
		wchar_t processName[MAX_PATH] = { 0 };
		DWORD size = MAX_PATH;
		if (QueryFullProcessImageNameW(hProc, 0, processName, &size)) 
		{
			std::wstring path(processName);
			std::transform(path.begin(), path.end(), path.begin(), ::towlower);
			if (path.find(L"explorer.exe") != std::wstring::npos) 
			{
				LONG_PTR style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
				if (style & WS_EX_TOOLWINDOW) 
				{
					CloseHandle(hProc);
					return true;
				}
			}
		}
		CloseHandle(hProc);
	}

	return false;
}