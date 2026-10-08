#pragma once
#include <windows.h>
#include <string>

std::wstring GetProcessNameFromHWND(HWND hwnd);
void CloseActiveWindow(HWND hwnd);
void CloseActiveProcess(HWND hwnd);
void ToggleFullscreen(HWND hwnd);
void RestoreFullscreenWindows();
void ToggleAlwaysOnTop(HWND hwnd);
void MinimizeWindow(HWND hwnd);
void CenterWindow(HWND hwnd);
void KillAllByProcessName(HWND hwnd);
bool IsSystemUIWindow(HWND hwnd);
bool IsManageableWindow(HWND hwnd);
bool IsFullscreenWindow(HWND hwnd);
bool HasFullscreenState(HWND hwnd);
