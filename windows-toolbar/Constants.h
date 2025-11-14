#pragma once
#include <windows.h>

// --- Layout Constants ---
constexpr int GAP                = 12;    // Gap between buttons
constexpr int TOOLBAR_HEIGHT     = 45;    // Height of the toolbar
constexpr int BUTTON_WIDTH       = 69;    // Width of each button
constexpr int BUTTON_HEIGHT      = 23;    // Height of each button
constexpr int CORNER_RADIUS      = 10;    // Corner radius of the toolbar
constexpr int ANIMATION_DURATION = 150;   // Duration of the animation

// --- Color Constants ---
constexpr COLORREF COLOR_BG      = RGB(18, 18, 24);    // #121218
constexpr COLORREF COLOR_CARD    = RGB(32, 32, 42);    // #20202A
constexpr COLORREF COLOR_PRIMARY = RGB(96, 165, 250);  // #60A5FA
constexpr COLORREF COLOR_TEXT    = RGB(248, 250, 252); // #F8FAFC
constexpr COLORREF COLOR_RED     = RGB(239, 68, 68);   // #EF4444
constexpr COLORREF COLOR_AMBER   = RGB(245, 158, 11);  // #F59E0B
constexpr COLORREF COLOR_GREEN   = RGB(16, 185, 129);  // #10B981
constexpr COLORREF COLOR_PURPLE  = RGB(139, 92, 246);  // #8B5CF6
constexpr COLORREF COLOR_PINK    = RGB(236, 72, 153);  // #EC4899
constexpr COLORREF COLOR_BLUE    = RGB(100, 150, 255); // #6496FF

// --- Icon Constants ---
constexpr const wchar_t* ICON_CLOSE         = L"✖";  // "Close Window"
constexpr const wchar_t* ICON_FULLSCREEN    = L"⛶"; // "Toggle Fullscreen"
constexpr const wchar_t* ICON_MINIMIZE      = L"—";  // "Minimize Window"
constexpr const wchar_t* ICON_PIN           = L"✯"; // "Toggle Always On Top"
constexpr const wchar_t* ICON_CENTER        = L"◎"; // "Center Window"
constexpr const wchar_t* ICON_KILL          = L"☠"; // "Kill All Processes"
constexpr const wchar_t* ICON_DOWN          = L"↓";  // "Minimize Toolbar"
constexpr const wchar_t* ICON_CLOSE_TOOLBAR = L"☒"; // "Close Toolbar"
constexpr const wchar_t* ICON_UP            = L"↑";  // "Restore Toolbar"

// --- Other Constants ---
constexpr ULONGLONG TOOLTIP_DELAY = 500;    // Delay for the tooltip
constexpr UINT WM_TRAYICON        = WM_USER + 1;    // Tray icon message
constexpr UINT TRAY_ICON_ID       = 1;    // Tray icon ID   