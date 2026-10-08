#pragma once
#include <windows.h>
#include <vector>
#include "Types.h"

extern HWND g_toolbar;
extern HWND g_active;
extern HWND g_minimizedToolbar;
extern HWND g_currentTooltipButton;
extern HWND g_tooltipWindow;
extern ULONGLONG g_tooltipShowTime;
extern bool g_trayIconAdded;
extern bool g_menuOpen;
extern ToolbarMode g_toolbarMode;
extern std::vector<ModernButton> g_buttons;
