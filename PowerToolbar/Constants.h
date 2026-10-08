#pragma once
#include <windows.h>

// Layout in device-independent pixels.
constexpr int GAP            = 8;
constexpr int TOOLBAR_HEIGHT = 64;
constexpr int BUTTON_WIDTH   = 42;
constexpr int BUTTON_HEIGHT  = 40;
constexpr int BUTTON_GAP     = 4;
constexpr int BRAND_WIDTH    = 166;
constexpr int CORNER_RADIUS  = 16;

// Axinomyus palette.
constexpr COLORREF COLOR_BG      = RGB(16, 20, 14);
constexpr COLORREF COLOR_CARD    = RGB(26, 32, 22);
constexpr COLORREF COLOR_HOVER   = RGB(43, 53, 33);
constexpr COLORREF COLOR_BORDER  = RGB(53, 63, 43);
constexpr COLORREF COLOR_PRIMARY = RGB(170, 255, 0);
constexpr COLORREF COLOR_TEXT    = RGB(242, 245, 235);
constexpr COLORREF COLOR_MUTED   = RGB(154, 166, 142);
constexpr COLORREF COLOR_RED     = RGB(255, 126, 119);

constexpr ULONGLONG TOOLTIP_DELAY = 450;
constexpr UINT WM_TRAYICON       = WM_APP + 1;
constexpr UINT WM_RESTORETOOLBAR = WM_APP + 2;
constexpr UINT TRAY_ICON_ID      = 1;
constexpr UINT TIMER_POSITION    = 1;
constexpr int MENU_SHOW          = 1000;
constexpr int MENU_EXIT          = 1001;
constexpr int MENU_COMPACT       = 1002;
constexpr int MENU_HIDE          = 1003;
constexpr int MENU_WEBSITE       = 1004;
constexpr int MENU_RESTOREWINDOW = 1005;
constexpr int MENU_SETTINGS      = 1006;

inline int Scale(int value, UINT dpi)
{
    return MulDiv(value, static_cast<int>(dpi), 96);
}
