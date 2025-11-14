#pragma once
#include <windows.h>
#include <vector>
#include "Types.h"

extern HWND g_toolbar;
extern HWND g_active;
extern bool g_visible;
extern ULONGLONG g_creationTime;
extern bool g_toolbarMinimized;
extern int g_toolbarWidth;
extern int g_minimizedWidth;
extern HWND g_tooltip;
extern HWND g_restoreButton;
extern HWND g_minimizedToolbar;
extern HWND g_currentTooltipButton;
extern HWND g_tooltipWindow;
extern ULONGLONG g_tooltipShowTime;
extern bool g_trayIconAdded;
extern std::vector<ModernButton> g_buttons;