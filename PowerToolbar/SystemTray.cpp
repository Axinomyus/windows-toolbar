#include "SystemTray.h"
#include "Globals.h"
#include "Constants.h"
#include "Toolbar.h"
#include "Tooltip.h"
#include "WindowOperations.h"
#include "Localization.h"
#include "resource.h"
#include <shellapi.h>

bool AddTrayIcon(HWND hwnd)
{
    NOTIFYICONDATAW icon = {};
    icon.cbSize = sizeof(icon);
    icon.hWnd = hwnd;
    icon.uID = TRAY_ICON_ID;
    icon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    icon.uCallbackMessage = WM_TRAYICON;
    icon.hIcon = static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_TRAYICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
    wcscpy_s(icon.szTip, L"PowerToolbar by Axinomyus");

    DWORD action = NIM_ADD;
    if (g_trayIconAdded)
    {
        action = NIM_MODIFY;
    }
    g_trayIconAdded = Shell_NotifyIconW(action, &icon) != FALSE;
    if (!g_trayIconAdded)
    {
        OutputDebugStringW(L"PowerToolbar: could not add the system tray icon.\n");
    }
    return g_trayIconAdded;
}

void RemoveTrayIcon()
{
    if (!g_trayIconAdded)
    {
        return;
    }

    NOTIFYICONDATAW icon = {};
    icon.cbSize = sizeof(icon);
    icon.hWnd = g_toolbar;
    icon.uID = TRAY_ICON_ID;
    Shell_NotifyIconW(NIM_DELETE, &icon);
    g_trayIconAdded = false;
}

void ShowTrayContextMenu(HWND hwnd, POINT point)
{
    HMENU menu = CreatePopupMenu();
    if (!menu)
    {
        OutputDebugStringW(L"PowerToolbar: could not create the tray menu.\n");
        return;
    }

    AppendMenuW(menu, MF_STRING, MENU_SHOW, Tr(Text::OpenToolbar));
    AppendMenuW(menu, MF_STRING, MENU_SETTINGS, Tr(Text::SettingsTitle));
    AppendMenuW(menu, MF_STRING, MENU_COMPACT, Tr(Text::Compact));
    AppendMenuW(menu, MF_STRING, MENU_HIDE, Tr(Text::HideToolbar));
    if (HasFullscreenState(g_active))
    {
        AppendMenuW(menu, MF_STRING, MENU_RESTOREWINDOW, Tr(Text::RestoreWindow));
    }
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, MENU_WEBSITE, Tr(Text::Website));
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, MENU_EXIT, Tr(Text::Exit));
    SetMenuDefaultItem(menu, MENU_SHOW, FALSE);
    HideButtonTooltip();

    HWND previousForeground = GetForegroundWindow();
    g_menuOpen = true;
    SetForegroundWindow(hwnd);
    const UINT command = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_RETURNCMD, point.x, point.y, 0, hwnd, nullptr);
    DestroyMenu(menu);
    PostMessageW(hwnd, WM_NULL, 0, 0);
    if (IsWindow(previousForeground))
    {
        SetForegroundWindow(previousForeground);
    }
    g_menuOpen = false;
    if (command != 0)
    {
        SendMessageW(g_toolbar, WM_COMMAND, command, 0);
    }
}
