#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <commctrl.h>
#include <shellapi.h>
#include "Constants.h"
#include "Globals.h"
#include "Drawing.h"
#include "Toolbar.h"
#include "Button.h"
#include "Tooltip.h"
#include "SystemTray.h"
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

LRESULT CALLBACK MinimizedToolbarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	switch (msg) 
	{
	case WM_CREATE: 
	{
		SetTimer(hwnd, 2, 100, NULL);
		break;
	}

	case WM_TIMER:
		if (wParam == 2) 
		{
			if (g_toolbarMinimized) 
			{
				UpdateMinimizedToolbarPosition();
			}
			else 
			{
				ShowWindow(hwnd, SW_HIDE);
			}
		}
		break;

	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT: 
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		PaintModernToolbar(hwnd, hdc);
		EndPaint(hwnd, &ps);
		break;
	}

	case WM_DESTROY:
		KillTimer(hwnd, 2);
		break;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK ModernToolbarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	switch (msg) 
	{
	case WM_CREATE: 
	{
		SetTimer(hwnd, 1, 100, NULL);

		g_tooltip = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED, TOOLTIPS_CLASSW, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hwnd, NULL, GetModuleHandle(NULL), NULL);

		if (g_tooltip) 
		{
			SendMessageW(g_tooltip, TTM_SETMAXTIPWIDTH, 0, 300);
			SendMessageW(g_tooltip, TTM_SETDELAYTIME, TTDT_INITIAL, 0);
			SendMessageW(g_tooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 10000);
			SendMessageW(g_tooltip, TTM_SETDELAYTIME, TTDT_RESHOW, 0);
			ShowWindow(g_tooltip, SW_SHOWNOACTIVATE);
		}

		for (int i = 0; i < (int)g_buttons.size(); ++i) 
		{
			HWND button = CreateWindowExW(0, L"ModernButton", L"",
				WS_CHILD | WS_VISIBLE,
				12 + i * (BUTTON_WIDTH + 12),
				(TOOLBAR_HEIGHT - BUTTON_HEIGHT) / 2,
				BUTTON_WIDTH, BUTTON_HEIGHT,
				hwnd, (HMENU)(INT_PTR)g_buttons[i].id, nullptr, nullptr);
		}

		HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
		int minimizedWidth	= 2 * (BUTTON_WIDTH + 12) + 24;
		g_minimizedToolbar	= CreateWindowExW(
			WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_NOACTIVATE,
			L"MinimizedToolbar", L"PowerToolbar Minimized",
			WS_POPUP,
			-1000, -1000, minimizedWidth, TOOLBAR_HEIGHT,
			nullptr, nullptr, hInstance, nullptr);

		if (g_minimizedToolbar) 
		{
			SetLayeredWindowAttributes(g_minimizedToolbar, 0, (BYTE)(255 * 0.95), LWA_ALPHA);
			ShowWindow(g_minimizedToolbar, SW_HIDE);

			HWND restoreBtn = CreateWindowExW(0, L"ModernButton", L"",
				WS_CHILD | WS_VISIBLE,
				12, (TOOLBAR_HEIGHT - BUTTON_HEIGHT) / 2,
				BUTTON_WIDTH, BUTTON_HEIGHT,
				g_minimizedToolbar, (HMENU)100, hInstance, nullptr);

			HWND closeBtn = CreateWindowExW(0, L"ModernButton", L"",
				WS_CHILD | WS_VISIBLE,
				12 + BUTTON_WIDTH + 12, (TOOLBAR_HEIGHT - BUTTON_HEIGHT) / 2,
				BUTTON_WIDTH, BUTTON_HEIGHT,
				g_minimizedToolbar, (HMENU)101, hInstance, nullptr);
		}
		break;
	}

	case WM_TIMER:
		UpdateToolbarPosition();

		if (g_currentTooltipButton && g_tooltipShowTime > 0) 
		{
			ULONGLONG currentTime = GetTickCount64();
			if (currentTime - g_tooltipShowTime >= TOOLTIP_DELAY) 
			{
				if (!g_tooltipWindow || !IsWindowVisible(g_tooltipWindow)) 
				{
					ShowButtonTooltip(g_currentTooltipButton);
				}
				g_tooltipShowTime = 0;
			}
		}
		break;

	case WM_TRAYICON:
		if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) 
		{
			POINT pt;
			GetCursorPos(&pt);
			ShowTrayContextMenu(hwnd, pt);
		}
		else if (lParam == WM_LBUTTONDBLCLK) 
		{
			ShowToolbar(true);
		}
		break;

	case WM_COMMAND:
		if (LOWORD(wParam) == 1000) 
		{
			ShowToolbar(true);
		}
		else if (LOWORD(wParam) == 1001) 
		{
			RemoveTrayIcon();
			PostQuitMessage(0);
		}
		break;

	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT: 
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		PaintModernToolbar(hwnd, hdc);
		EndPaint(hwnd, &ps);
		break;
	}

	case WM_DESTROY:
		KillTimer(hwnd, 1);
		HideButtonTooltip();
		RemoveTrayIcon();

		if (g_tooltip) 
		{
			DestroyWindow(g_tooltip);
			g_tooltip = nullptr;
		}

		if (g_tooltipWindow) 
		{
			wchar_t* textBuffer = (wchar_t*)GetWindowLongPtr(g_tooltipWindow, GWLP_USERDATA);
			if (textBuffer) delete[] textBuffer;
			DestroyWindow(g_tooltipWindow);
			g_tooltipWindow = nullptr;
		}

		if (g_minimizedToolbar) 
		{
			DestroyWindow(g_minimizedToolbar);
			g_minimizedToolbar = nullptr;
		}

		if (g_restoreButton) 
		{
			DestroyWindow(g_restoreButton);
			g_restoreButton = nullptr;
		}

		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) 
{
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC	= ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icex);

	WNDCLASSW wc		= { 0 };
	wc.lpfnWndProc		= ModernButtonProc;
	wc.hInstance		= hInstance;
	wc.lpszClassName	= L"ModernButton";
	wc.hCursor			= LoadCursor(nullptr, IDC_HAND);
	wc.hbrBackground	= CreateSolidBrush(COLOR_BG);
	RegisterClassW(&wc);

	wc = { 0 };
	wc.lpfnWndProc		= ModernToolbarProc;
	wc.hInstance		= hInstance;
	wc.lpszClassName	= L"ModernToolbar";
	wc.hbrBackground	= CreateSolidBrush(COLOR_BG);
	RegisterClassW(&wc);

	wc = { 0 };
	wc.lpfnWndProc		= MinimizedToolbarProc;
	wc.hInstance		= hInstance;
	wc.lpszClassName	= L"MinimizedToolbar";
	wc.hbrBackground	= CreateSolidBrush(COLOR_BG);
	RegisterClassW(&wc);

	g_toolbar = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_NOACTIVATE,
		L"ModernToolbar", L"PowerToolbar",
		WS_POPUP,
		100, 100, 800, TOOLBAR_HEIGHT,
		nullptr, nullptr, hInstance, nullptr);

	if (!g_toolbar)
	{
		return -1;
	}

	SetLayeredWindowAttributes(g_toolbar, 0, (BYTE)(255 * 0.95), LWA_ALPHA);

	SetWindowPos(g_toolbar, HWND_TOPMOST, -1000, -1000, 0, 0, SWP_NOSIZE);

	MSG msg;
	while (GetMessageW(&msg, nullptr, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return (int)msg.wParam;
}