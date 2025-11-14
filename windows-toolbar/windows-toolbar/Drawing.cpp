#include "Drawing.h"
#include "Constants.h"
#include <windows.h>

void DrawRoundedRect(HDC hdc, RECT rc, int radius, COLORREF color) 
{
	HBRUSH brush		= CreateSolidBrush(color);
	HPEN pen			= CreatePen(PS_SOLID, 1, color);
	HGDIOBJ oldBrush	= SelectObject(hdc, brush);
	HGDIOBJ oldPen		= SelectObject(hdc, pen);

	RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);
	SelectObject(hdc, oldBrush);
	SelectObject(hdc, oldPen);
	DeleteObject(brush);
	DeleteObject(pen);
}

void DrawModernButton(HDC hdc, RECT rc, const std::wstring& text,const std::wstring& icon, COLORREF color, bool hover, bool press)
{
	COLORREF bgColor	= COLOR_CARD;
	if (press) bgColor	= RGB(30, 30, 40);
	DrawRoundedRect(hdc, rc, 8, bgColor);

	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, color);

	HFONT iconFont	= CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI Symbol");
	HGDIOBJ oldFont = SelectObject(hdc, iconFont);

	DrawTextW(hdc, icon.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	SelectObject(hdc, oldFont);
	DeleteObject(iconFont);
}

void ApplyModernRoundRegion(HWND hwnd) 
{
	RECT rc;
	GetWindowRect(hwnd, &rc);

	int width	= rc.right - rc.left;
	int height	= rc.bottom - rc.top;
	HRGN region = CreateRoundRectRgn(0, 0, width, height, CORNER_RADIUS, CORNER_RADIUS);

	SetWindowRgn(hwnd, region, TRUE);
	DeleteObject(region);
}

void PaintModernToolbar(HWND hwnd, HDC hdc) 
{
	RECT rc;
	GetClientRect(hwnd, &rc);
	HBRUSH bgBrush = CreateSolidBrush(COLOR_BG);
	FillRect(hdc, &rc, bgBrush);
	DeleteObject(bgBrush);

	HPEN borderPen		= CreatePen(PS_SOLID, 1, RGB(55, 65, 81));
	HGDIOBJ oldPen		= SelectObject(hdc, borderPen);
	HGDIOBJ oldBrush	= SelectObject(hdc, GetStockObject(NULL_BRUSH));

	RoundRect(hdc, rc.left, rc.top, rc.right - 1, rc.bottom - 1, CORNER_RADIUS, CORNER_RADIUS);
	SelectObject(hdc, oldPen);
	SelectObject(hdc, oldBrush);
	DeleteObject(borderPen);
}