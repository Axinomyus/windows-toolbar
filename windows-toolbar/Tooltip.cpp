#include "Tooltip.h"
#include "Globals.h"
#include "Constants.h"
#include <commctrl.h>

std::wstring GetButtonTooltip(int buttonId)
{
	for (auto& b : g_buttons)
	{
		if (b.id == buttonId)
		{
			return b.tooltip;
		}
	}

	if (buttonId == 100) 
	{
		return L"Restore Toolbar";
	}

	return L"";
}

LRESULT CALLBACK TooltipWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		RECT rc;
		GetClientRect(hwnd, &rc);

		HBRUSH bgBrush = CreateSolidBrush(RGB(40, 40, 50));
		FillRect(hdc, &rc, bgBrush);
		DeleteObject(bgBrush);

		HPEN borderPen		= CreatePen(PS_SOLID, 1, RGB(80, 80, 90));
		HGDIOBJ oldPen		= SelectObject(hdc, borderPen);
		HGDIOBJ oldBrush	= SelectObject(hdc, GetStockObject(NULL_BRUSH));

		Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
		SelectObject(hdc, oldPen);
		SelectObject(hdc, oldBrush);
		DeleteObject(borderPen);

		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(248, 250, 252));
		HFONT font = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

		HGDIOBJ oldFont = SelectObject(hdc, font);

		InflateRect(&rc, -8, -4);
		wchar_t* text = (wchar_t*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (text) 
		{
			DrawTextW(hdc, text, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		}

		SelectObject(hdc, oldFont);
		DeleteObject(font);
		EndPaint(hwnd, &ps);

		return 0;
	}
	case WM_ERASEBKGND:
		return 1;
	default:
		return DefWindowProcW(hwnd, msg, wParam, lParam);
	}
}

void ShowButtonTooltip(HWND button) 
{
	if (!button)
	{
		return;
	}

	int buttonId				= GetDlgCtrlID(button);
	std::wstring tooltipText	= GetButtonTooltip(buttonId);

	if (tooltipText.empty()) 
	{
		return;
	}

	if (!g_tooltipWindow) 
	{
		WNDCLASSW wc		= { 0 };
		wc.lpfnWndProc		= TooltipWindowProc;
		wc.hInstance		= GetModuleHandle(NULL);
		wc.lpszClassName	= L"CustomTooltip";
		wc.hbrBackground	= CreateSolidBrush(RGB(40, 40, 50));
		RegisterClassW(&wc);

		g_tooltipWindow = CreateWindowExW(
			WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
			L"CustomTooltip", L"",
			WS_POPUP,
			0, 0, 200, 30,
			nullptr, nullptr, GetModuleHandle(NULL), nullptr);

		if (g_tooltipWindow) 
		{
			SetLayeredWindowAttributes(g_tooltipWindow, 0, 240, LWA_ALPHA);
		}
	}

	if (!g_tooltipWindow)
	{
		return;
	}

	wchar_t* oldBuffer = (wchar_t*)GetWindowLongPtr(g_tooltipWindow, GWLP_USERDATA);
	if (oldBuffer) 
	{
		delete[] oldBuffer;
	}

	size_t textLen		= tooltipText.length() + 1;
	wchar_t* textBuffer = new wchar_t[textLen];
	wcscpy_s(textBuffer, textLen, tooltipText.c_str());
	SetWindowLongPtr(g_tooltipWindow, GWLP_USERDATA, (LONG_PTR)textBuffer);

	HDC hdc		= GetDC(g_tooltipWindow);
	HFONT font	= CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
	HGDIOBJ oldFont = SelectObject(hdc, font);
	RECT textRect	= { 0, 0, 0, 0 };
	DrawTextW(hdc, tooltipText.c_str(), -1, &textRect, DT_CALCRECT | DT_SINGLELINE);
	SelectObject(hdc, oldFont);
	DeleteObject(font);
	ReleaseDC(g_tooltipWindow, hdc);

	int width	= textRect.right + 16;
	int height	= textRect.bottom + 8;

	POINT pt;
	GetCursorPos(&pt);
	int x = pt.x + 10;
	int y = pt.y + 25;

	int screenWidth		= GetSystemMetrics(SM_CXSCREEN);
	int screenHeight	= GetSystemMetrics(SM_CYSCREEN);
	if (x + width > screenWidth) x = screenWidth - width - 10;
	if (y + height > screenHeight) y = pt.y - height - 5;

	SetWindowPos(g_tooltipWindow, HWND_TOPMOST, x, y, width, height,
		SWP_SHOWWINDOW | SWP_NOACTIVATE);
	ShowWindow(g_tooltipWindow, SW_SHOWNOACTIVATE);
	UpdateWindow(g_tooltipWindow);

	g_currentTooltipButton = button;
}

void HideButtonTooltip() 
{
	if (g_tooltipWindow) 
	{
		wchar_t* textBuffer = (wchar_t*)GetWindowLongPtr(g_tooltipWindow, GWLP_USERDATA);
		if (textBuffer) 
		{
			delete[] textBuffer;
			SetWindowLongPtr(g_tooltipWindow, GWLP_USERDATA, 0);
		}

		ShowWindow(g_tooltipWindow, SW_HIDE);
	}

	g_currentTooltipButton	= nullptr;
	g_tooltipShowTime		= 0;
}