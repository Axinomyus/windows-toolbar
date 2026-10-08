#include "Button.h"
#include "Globals.h"
#include "Constants.h"
#include "Drawing.h"
#include "WindowOperations.h"
#include "Toolbar.h"
#include "Tooltip.h"
#include "Audio.h"
#include "Screenshot.h"
#include <windowsx.h>

namespace
{
    constexpr LONG_PTR HOVERED = 1;
    constexpr LONG_PTR PRESSED = 2;

    void ExecuteButton(int id, HWND target)
    {
        switch (id)
        {
        case 7:
            SetToolbarMode(ToolbarMode::Compact);
            return;
        case 8:
        case 101:
            SetToolbarMode(ToolbarMode::Hidden);
            return;
        case 100:
            SetToolbarMode(ToolbarMode::Expanded);
            return;
        }

        if (!IsManageableWindow(target))
        {
            return;
        }

        switch (id)
        {
        case 1:
            CloseActiveWindow(target);
            break;
        case 2:
            ToggleFullscreen(target);
            break;
        case 3:
            MinimizeWindow(target);
            break;
        case 4:
            ToggleAlwaysOnTop(target);
            break;
        case 5:
            CenterWindow(target);
            break;
        case 6:
            KillAllByProcessName(target);
            break;
        case 9:
            ToggleAppAudio(target);
            UpdateAudioButton(target, true);
            break;
        case 10:
            SaveWindowScreenshot(target);
            break;
        }
        UpdateToolbarPosition();
    }
}

LRESULT CALLBACK ModernButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LONG_PTR state = GetWindowLongPtrW(hwnd, 0);
    switch (msg)
    {
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_MOUSEMOVE:
        if (!(state & HOVERED))
        {
            SetWindowLongPtrW(hwnd, 0, state | HOVERED);
            TRACKMOUSEEVENT tracking = { sizeof(tracking), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tracking);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        if (g_currentTooltipButton != hwnd)
        {
            HideButtonTooltip();
            g_tooltipShowTime = GetTickCount64();
            g_currentTooltipButton = hwnd;
        }
        return 0;
    case WM_MOUSELEAVE:
        SetWindowLongPtrW(hwnd, 0, state & ~HOVERED);
        if (g_currentTooltipButton == hwnd)
        {
            HideButtonTooltip();
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_LBUTTONDOWN:
    {
        // A foreground change can race the polling timer. Resolve it before
        // capturing the target so a click never affects the previous app.
        UpdateToolbarPosition();
        HWND target = GetForegroundWindow();
        if (!IsManageableWindow(target))
        {
            target = nullptr;
        }
        SetWindowLongPtrW(hwnd, 0, state | PRESSED);
        SetWindowLongPtrW(hwnd, sizeof(LONG_PTR), reinterpret_cast<LONG_PTR>(target));
        DWORD processId = 0;
        GetWindowThreadProcessId(target, &processId);
        SetWindowLongPtrW(hwnd, 2 * sizeof(LONG_PTR), processId);
        SetCapture(hwnd);
        HideButtonTooltip();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_LBUTTONUP:
    {
        const bool pressed = (state & PRESSED) != 0;
        HWND target = reinterpret_cast<HWND>(GetWindowLongPtrW(hwnd, sizeof(LONG_PTR)));
        const DWORD capturedProcess = static_cast<DWORD>(GetWindowLongPtrW(hwnd, 2 * sizeof(LONG_PTR)));
        SetWindowLongPtrW(hwnd, 0, state & ~PRESSED);
        ReleaseCapture();
        InvalidateRect(hwnd, nullptr, FALSE);

        RECT bounds = {};
        GetClientRect(hwnd, &bounds);
        const POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        DWORD currentProcess = 0;
        GetWindowThreadProcessId(target, &currentProcess);
        if (pressed && PtInRect(&bounds, point) && capturedProcess == currentProcess)
        {
            ExecuteButton(GetDlgCtrlID(hwnd), target);
        }
        return 0;
    }
    case WM_CANCELMODE:
    case WM_CAPTURECHANGED:
        SetWindowLongPtrW(hwnd, 0, state & ~PRESSED);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_CONTEXTMENU:
        SendMessageW(GetParent(hwnd), msg, wParam, lParam);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
    {
        PAINTSTRUCT paint = {};
        HDC hdc = BeginPaint(hwnd, &paint);
        RECT bounds = {};
        GetClientRect(hwnd, &bounds);

        const int id = GetDlgCtrlID(hwnd);
        COLORREF color = COLOR_MUTED;
        for (const auto& button : g_buttons)
        {
            if (button.id == id)
            {
                color = button.color;
                break;
            }
        }

        HDC buffer = CreateCompatibleDC(hdc);
        HBITMAP bitmap = CreateCompatibleBitmap(hdc, bounds.right, bounds.bottom);
        HGDIOBJ previous = SelectObject(buffer, bitmap);
        bool active = id == 4 && GetWindowLongPtrW(hwnd, GWLP_USERDATA) != 0;
        if (id == 9)
        {
            active = GetWindowLongPtrW(hwnd, GWLP_USERDATA) == static_cast<LONG_PTR>(AppAudioState::Muted);
        }
        DrawModernButton(buffer, bounds, id, color, (state & HOVERED) != 0, (state & PRESSED) != 0, active, GetDpiForWindow(hwnd));
        BitBlt(hdc, 0, 0, bounds.right, bounds.bottom, buffer, 0, 0, SRCCOPY);
        SelectObject(buffer, previous);
        DeleteObject(bitmap);
        DeleteDC(buffer);
        EndPaint(hwnd, &paint);
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
