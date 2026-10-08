#include "Toolbar.h"
#include "Globals.h"
#include "Constants.h"
#include "WindowOperations.h"
#include "Drawing.h"
#include "Tooltip.h"
#include "SystemTray.h"
#include "Layout.h"
#include "Localization.h"
#include "SettingsWindow.h"
#include "Audio.h"
#include "FullscreenReveal.h"
#include <dwmapi.h>

namespace
{
    void HideToolbarWindows()
    {
        ShowWindow(g_toolbar, SW_HIDE);
        ShowWindow(g_minimizedToolbar, SW_HIDE);
        HideButtonTooltip();
    }

    int GetToolbarWidth(HWND hwnd)
    {
        if (hwnd == g_minimizedToolbar)
        {
            return 12 + 32 + 12 + 2 * (BUTTON_WIDTH + BUTTON_GAP) + 8;
        }

        int count = 0;
        for (const auto& button : g_buttons)
        {
            if (button.enabled)
            {
                ++count;
            }
        }
        return BRAND_WIDTH + count * (BUTTON_WIDTH + BUTTON_GAP) + 16;
    }

    void PositionToolbar(HWND toolbar, HWND target)
    {
        RECT frame = {};
        if (FAILED(DwmGetWindowAttribute(target, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(frame))))
        {
            if (!GetWindowRect(target, &frame))
            {
                ResetFullscreenReveal();
                HideToolbarWindows();
                return;
            }
        }

        MONITORINFO monitor = { sizeof(monitor) };
        if (!GetMonitorInfoW(MonitorFromWindow(target, MONITOR_DEFAULTTONEAREST), &monitor))
        {
            ResetFullscreenReveal();
            HideToolbarWindows();
            return;
        }

        UINT dpi = GetDpiForWindow(target);
        if (dpi == 0)
        {
            dpi = 96;
        }

        RECT bounds = {};
        const bool fullscreen = IsFullscreenWindow(target);
        if (IsZoomed(target) || fullscreen)
        {
            RECT area = monitor.rcWork;
            if (fullscreen)
            {
                area = monitor.rcMonitor;
            }
            bounds = CalculateTopCenteredBounds(area, Scale(GetToolbarWidth(toolbar), dpi), Scale(TOOLBAR_HEIGHT, dpi), Scale(26 + GAP, dpi));
            if (!UpdateFullscreenReveal(target, area, bounds, dpi))
            {
                HideToolbarWindows();
                return;
            }
        }
        else
        {
            ResetFullscreenReveal();
            bounds = CalculateToolbarBounds(frame, monitor.rcWork, Scale(GetToolbarWidth(toolbar), dpi), Scale(TOOLBAR_HEIGHT, dpi), Scale(GAP, dpi));
        }
        if (IsRectEmpty(&bounds))
        {
            HideToolbarWindows();
            return;
        }
        RECT current = {};
        GetWindowRect(toolbar, &current);
        if (!EqualRect(&bounds, &current) || !IsWindowVisible(toolbar))
        {
            HideButtonTooltip();
            SetWindowPos(toolbar, HWND_TOPMOST, bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top, SWP_NOACTIVATE | SWP_SHOWWINDOW);
            LayoutToolbar(toolbar);
            ApplyModernRoundRegion(toolbar);
            InvalidateRect(toolbar, nullptr, FALSE);
        }

        HWND pin = GetDlgItem(toolbar, 4);
        const LONG_PTR pinned = GetWindowLongPtrW(target, GWL_EXSTYLE) & WS_EX_TOPMOST;
        if (pin && GetWindowLongPtrW(pin, GWLP_USERDATA) != pinned)
        {
            SetWindowLongPtrW(pin, GWLP_USERDATA, pinned);
            InvalidateRect(pin, nullptr, FALSE);
        }
        UpdateAudioButton(target);
    }
}

void HideToolbarForCapture()
{
    HideToolbarWindows();
    ResetFullscreenReveal();
}

void UpdateAudioButton(HWND target, bool force)
{
    HWND button = GetDlgItem(g_toolbar, 9);
    static HWND previousTarget = nullptr;
    static DWORD previousProcess = 0;
    static ULONGLONG lastCheck = 0;
    if (!button || !IsWindowVisible(button))
    {
        previousTarget = nullptr;
        return;
    }
    DWORD processId = 0;
    GetWindowThreadProcessId(target, &processId);
    const ULONGLONG now = GetTickCount64();
    if (!force && previousTarget == target && previousProcess == processId && now - lastCheck < 500)
    {
        return;
    }
    previousTarget = target;
    previousProcess = processId;
    lastCheck = now;
    const LONG_PTR state = static_cast<LONG_PTR>(GetAppAudioState(target));
    if (GetWindowLongPtrW(button, GWLP_USERDATA) != state)
    {
        SetWindowLongPtrW(button, GWLP_USERDATA, state);
        InvalidateRect(button, nullptr, FALSE);
        if (g_currentTooltipButton == button)
        {
            HideButtonTooltip();
        }
    }
    SetWindowTextW(button, GetButtonTooltip(9).c_str());
}

void LayoutToolbar(HWND hwnd)
{
    const UINT dpi = GetDpiForWindow(hwnd);
    int x = BRAND_WIDTH;
    if (hwnd == g_minimizedToolbar)
    {
        x = 56;
    }

    if (hwnd == g_minimizedToolbar)
    {
        for (int id : {100, 101})
        {
            HWND button = GetDlgItem(hwnd, id);
            SetWindowPos(button, nullptr, Scale(x, dpi), Scale(12, dpi), Scale(BUTTON_WIDTH, dpi), Scale(BUTTON_HEIGHT, dpi), SWP_NOZORDER | SWP_NOACTIVATE);
            x += BUTTON_WIDTH + BUTTON_GAP;
        }
        SetWindowTextW(GetDlgItem(hwnd, 100), Tr(Text::Expand));
        SetWindowTextW(GetDlgItem(hwnd, 101), Tr(Text::HideToolbar));
        return;
    }
    for (const auto& definition : g_buttons)
    {
        HWND button = GetDlgItem(hwnd, definition.id);
        SetWindowTextW(button, definition.label.c_str());
        if (!definition.enabled)
        {
            ShowWindow(button, SW_HIDE);
            continue;
        }
        SetWindowPos(button, nullptr, Scale(x, dpi), Scale(12, dpi), Scale(BUTTON_WIDTH, dpi), Scale(BUTTON_HEIGHT, dpi), SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        x += BUTTON_WIDTH + BUTTON_GAP;
    }
}

void UpdateToolbarPosition()
{
    // Hidden is an explicit user choice, never a temporary visibility condition.
    if (g_toolbarMode == ToolbarMode::Hidden)
    {
        ResetFullscreenReveal();
        HideToolbarWindows();
        return;
    }

    if (g_menuOpen)
    {
        return;
    }

    HWND active = GetForegroundWindow();
    DWORD processId = 0;
    GetWindowThreadProcessId(active, &processId);
    if (processId == GetCurrentProcessId())
    {
        if (active == g_settingsWindow)
        {
            ResetFullscreenReveal();
            HideToolbarWindows();
        }
        return;
    }

    if (!IsManageableWindow(active))
    {
        ResetFullscreenReveal();
        if (!IsManageableWindow(g_active))
        {
            g_active = nullptr;
        }
        HideToolbarWindows();
        return;
    }

    if (g_active != active)
    {
        ResetFullscreenReveal();
        HideButtonTooltip();
        g_active = active;
    }

    HWND toolbar = g_toolbar;
    HWND other   = g_minimizedToolbar;
    if (g_toolbarMode == ToolbarMode::Compact)
    {
        toolbar = g_minimizedToolbar;
        other   = g_toolbar;
    }

    ShowWindow(other, SW_HIDE);
    PositionToolbar(toolbar, active);
}

void UpdateMinimizedToolbarPosition()
{
    UpdateToolbarPosition();
}

void SetToolbarMode(ToolbarMode mode)
{
    if (mode == ToolbarMode::Hidden && !g_trayIconAdded && !AddTrayIcon(g_toolbar))
    {
        MessageBoxW(g_toolbar, Tr(Text::TrayFailed), L"PowerToolbar", MB_OK | MB_ICONWARNING);
        return;
    }

    g_toolbarMode = mode;
    HideToolbarWindows();
    UpdateToolbarPosition();
}

void ShowToolbar(bool show)
{
    if (show)
    {
        SetToolbarMode(ToolbarMode::Expanded);
        return;
    }

    SetToolbarMode(ToolbarMode::Compact);
}
