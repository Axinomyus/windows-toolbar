#include "Tooltip.h"
#include "Globals.h"
#include "Constants.h"
#include "Localization.h"
#include "Audio.h"
#include <algorithm>

std::wstring GetButtonTooltip(int buttonId)
{
    if (buttonId == 9)
    {
        const auto state = static_cast<AppAudioState>(GetWindowLongPtrW(GetDlgItem(g_toolbar, 9), GWLP_USERDATA));
        if (state == AppAudioState::Muted)
        {
            return Tr(Text::UnmuteApp);
        }
        if (state == AppAudioState::Audible)
        {
            return Tr(Text::MuteApp);
        }
    }
    for (const auto& button : g_buttons)
    {
        if (button.id == buttonId)
        {
            return button.tooltip;
        }
    }
    if (buttonId == 100)
    {
        return Tr(Text::Expand);
    }
    if (buttonId == 101)
    {
        return Tr(Text::HideTip);
    }
    return {};
}

LRESULT CALLBACK TooltipWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_NCHITTEST:
        return HTTRANSPARENT;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
    {
        PAINTSTRUCT paint = {};
        HDC hdc = BeginPaint(hwnd, &paint);
        RECT bounds = {};
        GetClientRect(hwnd, &bounds);
        HBRUSH background = CreateSolidBrush(COLOR_CARD);
        FillRect(hdc, &bounds, background);
        DeleteObject(background);
        HBRUSH border = CreateSolidBrush(COLOR_BORDER);
        FrameRect(hdc, &bounds, border);
        DeleteObject(border);

        const UINT dpi = GetDpiForWindow(hwnd);
        HFONT font = CreateFontW(-Scale(12, dpi), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HGDIOBJ previousFont = SelectObject(hdc, font);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, COLOR_TEXT);
        wchar_t text[256] = {};
        GetWindowTextW(hwnd, text, 256);
        InflateRect(&bounds, -Scale(12, dpi), -Scale(6, dpi));
        DrawTextW(hdc, text, -1, &bounds, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
        SelectObject(hdc, previousFont);
        DeleteObject(font);
        EndPaint(hwnd, &paint);
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void ShowButtonTooltip(HWND button)
{
    if (!IsWindow(button) || !IsWindowVisible(button))
    {
        return;
    }

    const std::wstring text = GetButtonTooltip(GetDlgCtrlID(button));
    if (text.empty())
    {
        return;
    }
    if (!g_tooltipWindow)
    {
        g_tooltipWindow = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT, L"PowerToolbar.Tooltip", L"", WS_POPUP, 0, 0, 0, 0, g_toolbar, nullptr, GetModuleHandleW(nullptr), nullptr);
    }
    if (!g_tooltipWindow)
    {
        OutputDebugStringW(L"PowerToolbar: could not create tooltip.\n");
        return;
    }

    const UINT dpi = GetDpiForWindow(button);
    HDC hdc = GetDC(button);
    HFONT font = CreateFontW(-Scale(12, dpi), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HGDIOBJ previousFont = SelectObject(hdc, font);
    RECT textBounds = {};
    DrawTextW(hdc, text.c_str(), -1, &textBounds, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(hdc, previousFont);
    DeleteObject(font);
    ReleaseDC(button, hdc);

    RECT buttonBounds = {};
    GetWindowRect(button, &buttonBounds);
    MONITORINFO monitor = { sizeof(monitor) };
    if (!GetMonitorInfoW(MonitorFromWindow(button, MONITOR_DEFAULTTONEAREST), &monitor))
    {
        return;
    }

    const int width  = textBounds.right + Scale(24, dpi);
    const int height = textBounds.bottom + Scale(16, dpi);
    LONG x = buttonBounds.left + (buttonBounds.right - buttonBounds.left - width) / 2;
    LONG y = buttonBounds.bottom + Scale(18, dpi);
    x = (std::max)(monitor.rcWork.left, (std::min)(x, monitor.rcWork.right - width));
    if (y + height > monitor.rcWork.bottom)
    {
        y = buttonBounds.top - height - Scale(12, dpi);
    }
    y = (std::max)(y, monitor.rcWork.top);

    SetWindowTextW(g_tooltipWindow, text.c_str());
    SetWindowPos(g_tooltipWindow, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(g_tooltipWindow, nullptr, FALSE);
}

void HideButtonTooltip()
{
    if (g_tooltipWindow)
    {
        ShowWindow(g_tooltipWindow, SW_HIDE);
    }
    g_currentTooltipButton = nullptr;
    g_tooltipShowTime = 0;
}
