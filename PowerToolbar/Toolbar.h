#pragma once
#include <windows.h>
#include "Types.h"

void UpdateToolbarPosition();
void UpdateMinimizedToolbarPosition();
void SetToolbarMode(ToolbarMode mode);
void ShowToolbar(bool show);
void LayoutToolbar(HWND hwnd);
void UpdateAudioButton(HWND target, bool force = false);
void HideToolbarForCapture();
