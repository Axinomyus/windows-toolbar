#include "Globals.h"
#include "Constants.h"

HWND g_toolbar              = nullptr;
HWND g_active               = nullptr;
HWND g_minimizedToolbar     = nullptr;
HWND g_currentTooltipButton = nullptr;
HWND g_tooltipWindow        = nullptr;
ULONGLONG g_tooltipShowTime  = 0;
bool g_trayIconAdded        = false;
bool g_menuOpen             = false;
ToolbarMode g_toolbarMode  = ToolbarMode::Expanded;

std::vector<ModernButton> g_buttons;
