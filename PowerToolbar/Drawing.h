#pragma once
#include <windows.h>

void DrawModernButton(HDC hdc, RECT rc, int buttonId, COLORREF color, bool hover, bool press, bool active, UINT dpi);
void ApplyModernRoundRegion(HWND hwnd);
void PaintModernToolbar(HWND hwnd, HDC hdc);
