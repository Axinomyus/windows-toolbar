#pragma once
#include <windows.h>
#include <string>

void DrawRoundedRect(HDC hdc, RECT rc, int radius, COLORREF color);
void DrawModernButton(HDC hdc, RECT rc, const std::wstring& text, const std::wstring& icon, COLORREF color, bool hover, bool press);
void ApplyModernRoundRegion(HWND hwnd);
void PaintModernToolbar(HWND hwnd, HDC hdc);