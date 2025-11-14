#include "Globals.h"
#include "Constants.h"

HWND g_toolbar              = nullptr;
HWND g_active               = nullptr;
bool g_visible              = false;
ULONGLONG g_creationTime    = 0;
bool g_toolbarMinimized     = false;
int g_toolbarWidth          = 0;
int g_minimizedWidth        = 80;
HWND g_tooltip              = nullptr;
HWND g_restoreButton        = nullptr;
HWND g_minimizedToolbar     = nullptr;
HWND g_currentTooltipButton = nullptr;
HWND g_tooltipWindow        = nullptr;
ULONGLONG g_tooltipShowTime = 0;
bool g_trayIconAdded        = false;

std::vector<ModernButton> g_buttons = {
    {1, L"", ICON_CLOSE,         COLOR_RED,     L"Close Window"},
    {2, L"", ICON_FULLSCREEN,    COLOR_PRIMARY, L"Toggle Fullscreen"},
    {3, L"", ICON_MINIMIZE,      COLOR_AMBER,   L"Minimize Window"},
    {4, L"", ICON_PIN,           COLOR_GREEN,   L"Toggle Always On Top"},
    {5, L"", ICON_CENTER,        COLOR_PURPLE,  L"Center Window"},
    {6, L"", ICON_KILL,          COLOR_PINK,    L"Kill All Processes"},
    {7, L"", ICON_DOWN,          COLOR_BLUE,    L"Minimize Toolbar"},
    {8, L"", ICON_CLOSE_TOOLBAR, COLOR_RED,     L"Close Toolbar"}
};