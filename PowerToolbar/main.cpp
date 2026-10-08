#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <shobjidl.h>
#include "Constants.h"
#include "Globals.h"
#include "Drawing.h"
#include "Toolbar.h"
#include "Button.h"
#include "Tooltip.h"
#include "SystemTray.h"
#include "WindowOperations.h"
#include "Settings.h"
#include "SettingsWindow.h"
#include "FullscreenReveal.h"
#include "resource.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")

namespace
{
    const UINT taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");

    bool CreateButtons(HWND hwnd, HINSTANCE instance, bool compact)
    {
        if (compact)
        {
            if (!CreateWindowExW(0, L"PowerToolbar.Button", L"Expand PowerToolbar", WS_CHILD | WS_VISIBLE, 0, 0, 1, 1, hwnd, reinterpret_cast<HMENU>(100), instance, nullptr) ||
                !CreateWindowExW(0, L"PowerToolbar.Button", L"Hide toolbar", WS_CHILD | WS_VISIBLE, 0, 0, 1, 1, hwnd, reinterpret_cast<HMENU>(101), instance, nullptr))
            {
                return false;
            }
        }
        else
        {
            for (const auto& button : g_buttons)
            {
                if (!CreateWindowExW(0, L"PowerToolbar.Button", button.label.c_str(), WS_CHILD | WS_VISIBLE, 0, 0, 1, 1, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(button.id)), instance, nullptr))
                {
                    return false;
                }
            }
        }
        return true;
    }

    void HandleCommand(int command)
    {
        switch (command)
        {
        case MENU_SHOW:
            SetToolbarMode(ToolbarMode::Expanded);
            break;
        case MENU_COMPACT:
            SetToolbarMode(ToolbarMode::Compact);
            break;
        case MENU_HIDE:
            SetToolbarMode(ToolbarMode::Hidden);
            break;
        case MENU_RESTOREWINDOW:
            if (HasFullscreenState(g_active))
            {
                ToggleFullscreen(g_active);
                UpdateToolbarPosition();
            }
            break;
        case MENU_WEBSITE:
            OpenProductWebsite(g_toolbar);
            break;
        case MENU_SETTINGS:
            ShowSettingsWindow();
            break;
        case MENU_EXIT:
            DestroyWindow(g_toolbar);
            break;
        }
    }

    LRESULT CALLBACK ToolbarProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == taskbarCreated)
        {
            g_trayIconAdded = false;
            if (!AddTrayIcon(g_toolbar) && g_toolbarMode == ToolbarMode::Hidden)
            {
                SetToolbarMode(ToolbarMode::Expanded);
            }
            return 0;
        }

        switch (message)
        {
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case WM_LBUTTONUP:
            if (GET_X_LPARAM(lParam) < Scale(BRAND_WIDTH, GetDpiForWindow(hwnd)))
            {
                ShowSettingsWindow();
            }
            return 0;
        case WM_RESTORETOOLBAR:
            SetToolbarMode(ToolbarMode::Expanded);
            return 0;
        case WM_TIMER:
            if (wParam == TIMER_POSITION)
            {
                UpdateToolbarPosition();
                if (g_currentTooltipButton && g_tooltipShowTime != 0 && GetTickCount64() - g_tooltipShowTime >= TOOLTIP_DELAY)
                {
                    ShowButtonTooltip(g_currentTooltipButton);
                    g_tooltipShowTime = 0;
                }
            }
            return 0;
        case WM_DPICHANGED:
            LayoutToolbar(hwnd);
            ApplyModernRoundRegion(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_DISPLAYCHANGE:
        case WM_SETTINGCHANGE:
            UpdateToolbarPosition();
            return 0;
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU)
            {
                POINT point = {};
                GetCursorPos(&point);
                ShowTrayContextMenu(hwnd, point);
            }
            else if (lParam == WM_LBUTTONDBLCLK || lParam == NIN_KEYSELECT)
            {
                SetToolbarMode(ToolbarMode::Expanded);
            }
            return 0;
        case WM_CONTEXTMENU:
        {
            POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            if (point.x == -1 && point.y == -1)
            {
                GetCursorPos(&point);
            }
            ShowTrayContextMenu(hwnd, point);
            return 0;
        }
        case WM_COMMAND:
            HandleCommand(LOWORD(wParam));
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT:
        {
            PAINTSTRUCT paint = {};
            HDC hdc = BeginPaint(hwnd, &paint);
            RECT bounds = {};
            GetClientRect(hwnd, &bounds);
            HDC buffer = CreateCompatibleDC(hdc);
            HBITMAP bitmap = CreateCompatibleBitmap(hdc, bounds.right, bounds.bottom);
            HGDIOBJ previous = SelectObject(buffer, bitmap);
            PaintModernToolbar(hwnd, buffer);
            BitBlt(hdc, 0, 0, bounds.right, bounds.bottom, buffer, 0, 0, SRCCOPY);
            SelectObject(buffer, previous);
            DeleteObject(bitmap);
            DeleteDC(buffer);
            EndPaint(hwnd, &paint);
            return 0;
        }
        case WM_CLOSE:
            if (hwnd == g_toolbar)
            {
                DestroyWindow(hwnd);
            }
            return 0;
        case WM_DESTROY:
            if (hwnd == g_toolbar)
            {
                KillTimer(hwnd, TIMER_POSITION);
                if (g_settingsWindow)
                {
                    DestroyWindow(g_settingsWindow);
                }
                RestoreFullscreenWindows();
                HideButtonTooltip();
                RemoveTrayIcon();
                if (g_tooltipWindow)
                {
                    DestroyWindow(g_tooltipWindow);
                    g_tooltipWindow = nullptr;
                }
                if (g_minimizedToolbar)
                {
                    DestroyWindow(g_minimizedToolbar);
                    g_minimizedToolbar = nullptr;
                }
                PostQuitMessage(0);
            }
            return 0;
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    bool RegisterWindowClasses(HINSTANCE instance)
    {
        WNDCLASSEXW windowClass = { sizeof(windowClass) };
        windowClass.hInstance = instance;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_MAINICON));
        windowClass.hIconSm = windowClass.hIcon;
        windowClass.lpfnWndProc = ToolbarProc;
        windowClass.lpszClassName = L"PowerToolbar.Window";
        if (!RegisterClassExW(&windowClass))
        {
            return false;
        }

        windowClass.lpszClassName = L"PowerToolbar.Compact";
        if (!RegisterClassExW(&windowClass))
        {
            return false;
        }

        windowClass.lpfnWndProc = TooltipWindowProc;
        windowClass.lpszClassName = L"PowerToolbar.Tooltip";
        if (!RegisterClassExW(&windowClass))
        {
            return false;
        }

        windowClass.lpfnWndProc = ModernButtonProc;
        windowClass.cbWndExtra = 3 * sizeof(LONG_PTR);
        windowClass.lpszClassName = L"PowerToolbar.Button";
        windowClass.hCursor = LoadCursorW(nullptr, IDC_HAND);
        return RegisterClassExW(&windowClass) != 0;
    }
}

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int)
{
    LoadSettings();
    RefreshToolbarButtons();
    HANDLE singleInstance = CreateMutexW(nullptr, FALSE, L"Local\\Axinomyus.PowerToolbar");
    if (!singleInstance)
    {
        MessageBoxW(nullptr, Tr(Text::InitFailed), L"PowerToolbar", MB_OK | MB_ICONERROR);
        return 1;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        HWND existing = FindWindowW(L"PowerToolbar.Window", nullptr);
        if (existing)
        {
            PostMessageW(existing, WM_RESTORETOOLBAR, 0, 0);
        }
        CloseHandle(singleInstance);
        return 0;
    }

    SetCurrentProcessExplicitAppUserModelID(L"Axinomyus.PowerToolbar");
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(comResult))
    {
        MessageBoxW(nullptr, Tr(Text::InitFailed), L"PowerToolbar", MB_OK | MB_ICONERROR);
        CloseHandle(singleInstance);
        return 1;
    }
    Gdiplus::GdiplusStartupInput startup;
    ULONG_PTR graphicsToken = 0;
    if (Gdiplus::GdiplusStartup(&graphicsToken, &startup, nullptr) != Gdiplus::Ok)
    {
        MessageBoxW(nullptr, Tr(Text::InitFailed), L"PowerToolbar", MB_OK | MB_ICONERROR);
        CoUninitialize();
        CloseHandle(singleInstance);
        return 1;
    }

    int result = 1;
    if (RegisterWindowClasses(instance))
    {
        const DWORD extendedStyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
        g_toolbar = CreateWindowExW(extendedStyle, L"PowerToolbar.Window", L"PowerToolbar", WS_POPUP | WS_CLIPCHILDREN, 0, 0, 550, TOOLBAR_HEIGHT, nullptr, nullptr, instance, nullptr);
        g_minimizedToolbar = CreateWindowExW(extendedStyle, L"PowerToolbar.Compact", L"PowerToolbar - Compact", WS_POPUP | WS_CLIPCHILDREN, 0, 0, 156, TOOLBAR_HEIGHT, g_toolbar, nullptr, instance, nullptr);
        if (g_toolbar && g_minimizedToolbar && InitializeFullscreenReveal(instance) && CreateButtons(g_toolbar, instance, false) && CreateButtons(g_minimizedToolbar, instance, true) && SetTimer(g_toolbar, TIMER_POSITION, 100, nullptr))
        {
            if (!AddTrayIcon(g_toolbar))
            {
                MessageBoxW(g_toolbar, Tr(Text::TrayFailed), L"PowerToolbar", MB_OK | MB_ICONINFORMATION);
            }
            LayoutToolbar(g_toolbar);
            LayoutToolbar(g_minimizedToolbar);
            if (GetSettings().startHidden)
            {
                SetToolbarMode(ToolbarMode::Hidden);
            }
            UpdateToolbarPosition();
            MSG message = {};
            BOOL status = 0;
            while ((status = GetMessageW(&message, nullptr, 0, 0)) > 0)
            {
                if (g_settingsWindow && IsDialogMessageW(g_settingsWindow, &message))
                {
                    continue;
                }
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            if (status == 0)
            {
                result = static_cast<int>(message.wParam);
            }
            else
            {
                OutputDebugStringW(L"PowerToolbar: GetMessage failed.\n");
                DestroyWindow(g_toolbar);
            }
        }
        else
        {
            MessageBoxW(nullptr, Tr(Text::InitFailed), L"PowerToolbar", MB_OK | MB_ICONERROR);
            if (g_toolbar)
            {
                DestroyWindow(g_toolbar);
            }
        }
    }
    else
    {
        MessageBoxW(nullptr, Tr(Text::InitFailed), L"PowerToolbar", MB_OK | MB_ICONERROR);
    }

    Gdiplus::GdiplusShutdown(graphicsToken);
    CoUninitialize();
    CloseHandle(singleInstance);
    return result;
}
