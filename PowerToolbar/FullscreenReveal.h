#pragma once
#include <windows.h>

bool InitializeFullscreenReveal(HINSTANCE instance);
void ResetFullscreenReveal();
bool UpdateFullscreenReveal(HWND target, const RECT& area, const RECT& toolbarBounds, UINT dpi);
