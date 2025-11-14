#pragma once
#include <windows.h>
#include <string>

std::wstring GetButtonTooltip(int buttonId);
void ShowButtonTooltip(HWND button);
void HideButtonTooltip();
LRESULT CALLBACK TooltipWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);