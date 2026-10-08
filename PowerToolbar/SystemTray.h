#pragma once
#include <windows.h>

bool AddTrayIcon(HWND hwnd);
void RemoveTrayIcon();
void ShowTrayContextMenu(HWND hwnd, POINT pt);
