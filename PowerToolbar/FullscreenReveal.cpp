#include "FullscreenReveal.h"
#include "Constants.h"
#include "Drawing.h"
#include "Globals.h"
#include "Layout.h"
#include "Localization.h"
#include "Toolbar.h"
#include "WindowOperations.h"
#include <windowsx.h>

namespace
{
    constexpr ULONGLONG REVEAL_DELAY = 200;
    constexpr ULONGLONG HIDE_DELAY   = 750;

    HWND revealWindow        = nullptr;
    HWND revealTarget        = nullptr;
    DWORD targetProcess      = 0;
    RECT revealArea          = {};
    ULONGLONG edgeEnteredAt  = 0;
    ULONGLONG lastHoveredAt  = 0;
    bool panelOpen           = false;
    bool hovered             = false;
    bool pressed             = false;

    LRESULT CALLBACK RevealProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case WM_MOUSEMOVE:
            if (!hovered)
            {
                hovered = true;
                TRACKMOUSEEVENT tracking = {sizeof(tracking), TME_LEAVE, hwnd, 0};
                TrackMouseEvent(&tracking);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_MOUSELEAVE:
            hovered = false;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_LBUTTONDOWN:
            pressed = true;
            SetCapture(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_LBUTTONUP:
        {
            const bool wasPressed = pressed;
            pressed = false;
            ReleaseCapture();
            RECT bounds = {};
            GetClientRect(hwnd, &bounds);
            const POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            DWORD processId = 0;
            GetWindowThreadProcessId(revealTarget, &processId);
            if (wasPressed && PtInRect(&bounds, point) && GetForegroundWindow() == revealTarget &&
                processId == targetProcess && g_toolbarMode != ToolbarMode::Hidden &&
                IsManageableWindow(revealTarget) && (IsZoomed(revealTarget) || IsFullscreenWindow(revealTarget)))
            {
                panelOpen = !panelOpen;
                lastHoveredAt = GetTickCount64();
                UpdateToolbarPosition();
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_CANCELMODE:
            ReleaseCapture();
            [[fallthrough]];
        case WM_CAPTURECHANGED:
            pressed = false;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT:
        {
            PAINTSTRUCT paint = {};
            HDC hdc = BeginPaint(hwnd, &paint);
            RECT bounds = {};
            GetClientRect(hwnd, &bounds);
            int glyph = 7;
            if (panelOpen)
            {
                glyph = 100;
            }
            DrawModernButton(hdc, bounds, glyph, COLOR_PRIMARY, hovered, pressed, false, GetDpiForWindow(hwnd));
            EndPaint(hwnd, &paint);
            return 0;
        }
        case WM_DESTROY:
            revealWindow = nullptr;
            return 0;
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

bool InitializeFullscreenReveal(HINSTANCE instance)
{
    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc   = RevealProc;
    windowClass.hInstance     = instance;
    windowClass.hCursor       = LoadCursorW(nullptr, IDC_HAND);
    windowClass.lpszClassName = L"PowerToolbar.Reveal";
    if (!RegisterClassW(&windowClass))
    {
        return false;
    }
    revealWindow = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, windowClass.lpszClassName, Tr(Text::RevealToolbar), WS_POPUP, 0, 0, 48, 26, g_toolbar, nullptr, instance, nullptr);
    return revealWindow != nullptr;
}

void ResetFullscreenReveal()
{
    if (revealWindow)
    {
        if (GetCapture() == revealWindow)
        {
            ReleaseCapture();
        }
        ShowWindow(revealWindow, SW_HIDE);
    }
    revealTarget  = nullptr;
    targetProcess = 0;
    edgeEnteredAt = 0;
    lastHoveredAt = 0;
    panelOpen     = false;
    hovered       = false;
    pressed       = false;
}

bool UpdateFullscreenReveal(HWND target, const RECT& area, const RECT& toolbarBounds, UINT dpi)
{
    DWORD processId = 0;
    GetWindowThreadProcessId(target, &processId);
    if (target != revealTarget || processId != targetProcess || !EqualRect(&area, &revealArea))
    {
        ResetFullscreenReveal();
        revealTarget  = target;
        targetProcess = processId;
        revealArea    = area;
    }

    POINT cursor = {};
    if (!revealWindow || IsRectEmpty(&toolbarBounds) || !GetCursorPos(&cursor))
    {
        ResetFullscreenReveal();
        return false;
    }
    const RECT edge   = CalculateTopCenteredBounds(area, Scale(84, dpi), Scale(4, dpi), 0);
    const RECT arrow  = CalculateTopCenteredBounds(area, Scale(48, dpi), Scale(26, dpi), 0);
    const ULONGLONG now = GetTickCount64();
    const bool dragging = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) || (GetAsyncKeyState(VK_RBUTTON) & 0x8000) || (GetAsyncKeyState(VK_MBUTTON) & 0x8000);
    if (PtInRect(&edge, cursor) && !dragging)
    {
        if (edgeEnteredAt == 0)
        {
            edgeEnteredAt = now;
        }
        if (now - edgeEnteredAt >= REVEAL_DELAY)
        {
            lastHoveredAt = now;
        }
    }
    else
    {
        edgeEnteredAt = 0;
    }

    if (IsWindowVisible(revealWindow))
    {
        RECT interaction = arrow;
        if (panelOpen)
        {
            // Include the short gap so crossing from the arrow to the toolbar is safe.
            UnionRect(&interaction, &arrow, &toolbarBounds);
            InflateRect(&interaction, Scale(8, dpi), Scale(8, dpi));
        }
        HWND capture = GetCapture();
        const bool interacting = capture && (capture == revealWindow || IsChild(g_toolbar, capture) || IsChild(g_minimizedToolbar, capture));
        if (PtInRect(&interaction, cursor) || interacting)
        {
            lastHoveredAt = now;
        }
    }
    if (lastHoveredAt == 0 || now - lastHoveredAt > HIDE_DELAY)
    {
        panelOpen = false;
        hovered = false;
        ShowWindow(revealWindow, SW_HIDE);
        return false;
    }

    RECT current = {};
    GetWindowRect(revealWindow, &current);
    if (!EqualRect(&current, &arrow) || !IsWindowVisible(revealWindow))
    {
        SetWindowPos(revealWindow, HWND_TOPMOST, arrow.left, arrow.top, arrow.right - arrow.left, arrow.bottom - arrow.top, SWP_NOACTIVATE | SWP_SHOWWINDOW);
        ApplyModernRoundRegion(revealWindow);
        InvalidateRect(revealWindow, nullptr, FALSE);
    }
    Text label = Text::RevealToolbar;
    if (panelOpen)
    {
        label = Text::CollapseToolbar;
    }
    SetWindowTextW(revealWindow, Tr(label));
    return panelOpen;
}
