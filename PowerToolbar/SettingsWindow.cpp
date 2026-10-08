#include "SettingsWindow.h"
#include "Settings.h"
#include "Constants.h"
#include "Globals.h"
#include "Toolbar.h"
#include "Tooltip.h"
#include "resource.h"
#include <dwmapi.h>
#include <shellapi.h>
#include <windowsx.h>
#include <algorithm>
#include <string>
#include <vector>

HWND g_settingsWindow = nullptr;

namespace
{
    constexpr int NAV_GENERAL  = 2000;
    constexpr int NAV_TOOLBAR  = 2001;
    constexpr int NAV_ABOUT    = 2002;
    constexpr int LANGUAGE    = 2010;
    constexpr int STARTUP     = 2011;
    constexpr int START_HIDDEN = 2012;
    constexpr int BUTTON_LIST = 2020;
    constexpr int MOVE_UP     = 2021;
    constexpr int MOVE_DOWN   = 2022;
    constexpr int TOGGLE      = 2023;
    constexpr int RESET       = 2024;
    constexpr int SAVE        = 2030;
    constexpr int CANCEL      = 2031;
    constexpr int WEBSITE     = 2032;
    constexpr int TITLE       = 2040;
    constexpr int SUBTITLE    = 2041;
    constexpr int LANG_LABEL  = 2042;
    constexpr int GENERAL_NOTE = 2043;
    constexpr int AUDIO_NOTE  = 2044;
    constexpr int ABOUT_BODY  = 2045;
    constexpr int VERSION     = 2046;
    constexpr int PRIVACY     = 2047;
    constexpr int STATUS      = 2048;

    struct ControlLayout
    {
        int id;
        int x;
        int y;
        int width;
        int height;
        int page;
    };

    constexpr ControlLayout layouts[] =
    {
        {NAV_GENERAL, 20, 164, 180, 46, -1}, {NAV_TOOLBAR, 20, 220, 180, 46, -1},
        {NAV_ABOUT, 20, 276, 180, 46, -1},
        {TITLE, 248, 38, 600, 38, -1}, {SUBTITLE, 248, 84, 600, 44, -1},
        {LANG_LABEL, 248, 148, 590, 28, 0}, {LANGUAGE, 248, 186, 450, 240, 0},
        {STARTUP, 248, 256, 590, 48, 0}, {START_HIDDEN, 248, 318, 590, 48, 0},
        {GENERAL_NOTE, 248, 416, 590, 106, 0},
        {BUTTON_LIST, 248, 146, 416, 340, 1}, {MOVE_UP, 684, 146, 182, 44, 1},
        {MOVE_DOWN, 684, 202, 182, 44, 1}, {TOGGLE, 684, 258, 182, 44, 1},
        {RESET, 684, 386, 182, 44, 1}, {AUDIO_NOTE, 248, 504, 610, 62, 1},
        {ABOUT_BODY, 248, 156, 600, 180, 2}, {VERSION, 248, 346, 600, 30, 2},
        {PRIVACY, 248, 390, 600, 64, 2}, {WEBSITE, 248, 474, 360, 46, 2},
        {STATUS, 248, 595, 250, 32, -1}, {CANCEL, 508, 588, 132, 42, -1},
        {SAVE, 654, 588, 212, 42, -1}
    };

    AppSettings draft;
    bool draftStartup = false;
    int selectedPage = 0;
    int scrollX = 0;
    int scrollY = 0;
    HFONT normalFont = nullptr;
    HFONT titleFont = nullptr;
    HFONT smallFont = nullptr;
    HBRUSH backgroundBrush = nullptr;
    HBRUSH cardBrush = nullptr;

    const wchar_t* Local(Text text)
    {
        return Translate(text, draft.language);
    }

    void Fill(HDC hdc, const RECT& bounds, COLORREF color)
    {
        HBRUSH brush = CreateSolidBrush(color);
        FillRect(hdc, &bounds, brush);
        DeleteObject(brush);
    }

    void Label(HDC hdc, RECT bounds, const wchar_t* text, HFONT font, COLORREF color, UINT format = DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX)
    {
        HGDIOBJ previous = SelectObject(hdc, font);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, color);
        DrawTextW(hdc, text, -1, &bounds, format);
        SelectObject(hdc, previous);
    }

    void ClearStatus()
    {
        SetWindowTextW(GetDlgItem(g_settingsWindow, STATUS), L"");
    }

    void UpdateToolHint(int selection)
    {
        Text hint = Text::AudioHint;
        if (selection >= 0 && selection < static_cast<int>(draft.buttons.size()) && draft.buttons[selection].id == 10)
        {
            hint = Text::ScreenshotHint;
        }
        SetWindowTextW(GetDlgItem(g_settingsWindow, AUDIO_NOTE), Local(hint));
    }

    void PopulateButtons(int selection)
    {
        HWND list = GetDlgItem(g_settingsWindow, BUTTON_LIST);
        SendMessageW(list, LB_RESETCONTENT, 0, 0);
        for (const auto& button : draft.buttons)
        {
            std::wstring text = Local(ButtonLabel(button.id));
            text += L" - ";
            if (button.enabled)
            {
                text += Local(Text::Enabled);
            }
            else
            {
                text += Local(Text::Disabled);
            }
            SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
        }
        SendMessageW(list, LB_SETCURSEL, selection, 0);
        EnableWindow(GetDlgItem(g_settingsWindow, MOVE_UP), selection > 0);
        EnableWindow(GetDlgItem(g_settingsWindow, MOVE_DOWN), selection >= 0 && selection < static_cast<int>(draft.buttons.size()) - 1);
        UpdateToolHint(selection);
    }

    void RefreshText()
    {
        const std::pair<int, Text> labels[] =
        {
            {NAV_GENERAL, Text::General}, {NAV_TOOLBAR, Text::Toolbar}, {NAV_ABOUT, Text::About},
            {LANG_LABEL, Text::LanguageLabel}, {STARTUP, Text::Startup}, {START_HIDDEN, Text::StartHidden},
            {GENERAL_NOTE, Text::FullscreenHint}, {MOVE_UP, Text::MoveUp}, {MOVE_DOWN, Text::MoveDown},
            {TOGGLE, Text::ToggleButton}, {RESET, Text::ResetLayout}, {AUDIO_NOTE, Text::AudioHint},
            {ABOUT_BODY, Text::AboutBody}, {PRIVACY, Text::Privacy}, {WEBSITE, Text::Website},
            {SAVE, Text::Save}, {CANCEL, Text::Cancel}
        };
        SetWindowTextW(g_settingsWindow, Local(Text::SettingsTitle));
        for (const auto& label : labels)
        {
            SetWindowTextW(GetDlgItem(g_settingsWindow, label.first), Local(label.second));
        }
        std::wstring version = std::wstring(Local(Text::Version)) + L"   /   " + Local(Text::License);
        SetWindowTextW(GetDlgItem(g_settingsWindow, VERSION), version.c_str());
        const int selection = static_cast<int>(SendDlgItemMessageW(g_settingsWindow, BUTTON_LIST, LB_GETCURSEL, 0, 0));
        PopulateButtons((std::max)(selection, 0));
        Text title = Text::General;
        Text subtitle = Text::GeneralHint;
        if (selectedPage == 1)
        {
            title = Text::LayoutTitle;
            subtitle = Text::LayoutHint;
        }
        else if (selectedPage == 2)
        {
            title = Text::About;
            subtitle = Text::Tagline;
        }
        SetWindowTextW(GetDlgItem(g_settingsWindow, TITLE), Local(title));
        SetWindowTextW(GetDlgItem(g_settingsWindow, SUBTITLE), Local(subtitle));
        RedrawWindow(g_settingsWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
    }

    void LayoutControls()
    {
        const UINT dpi = GetDpiForWindow(g_settingsWindow);
        for (const auto& layout : layouts)
        {
            HWND control = GetDlgItem(g_settingsWindow, layout.id);
            SetWindowPos(control, nullptr, Scale(layout.x, dpi) - scrollX, Scale(layout.y, dpi) - scrollY, Scale(layout.width, dpi), Scale(layout.height, dpi), SWP_NOZORDER | SWP_NOACTIVATE);
            int visibility = SW_HIDE;
            if (layout.page < 0 || layout.page == selectedPage)
            {
                visibility = SW_SHOWNA;
            }
            ShowWindow(control, visibility);
        }
        SendDlgItemMessageW(g_settingsWindow, BUTTON_LIST, LB_SETITEMHEIGHT, 0, Scale(33, dpi));
        SendDlgItemMessageW(g_settingsWindow, LANGUAGE, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), Scale(34, dpi));
        SendDlgItemMessageW(g_settingsWindow, LANGUAGE, CB_SETITEMHEIGHT, 0, Scale(32, dpi));
    }

    void UpdateScrollbars()
    {
        static bool updating = false;
        if (updating)
        {
            return;
        }
        updating = true;
        // Measure the available area without existing bars, so growing the
        // window can remove both bars instead of keeping each other visible.
        ShowScrollBar(g_settingsWindow, SB_BOTH, FALSE);
        RECT bounds = {};
        GetClientRect(g_settingsWindow, &bounds);
        const UINT dpi = GetDpiForWindow(g_settingsWindow);
        SCROLLINFO horizontal = {sizeof(horizontal), SIF_RANGE | SIF_PAGE | SIF_POS, 0, Scale(890, dpi) - 1, static_cast<UINT>(bounds.right), scrollX};
        SCROLLINFO vertical = {sizeof(vertical), SIF_RANGE | SIF_PAGE | SIF_POS, 0, Scale(650, dpi) - 1, static_cast<UINT>(bounds.bottom), scrollY};
        SetScrollInfo(g_settingsWindow, SB_HORZ, &horizontal, TRUE);
        SetScrollInfo(g_settingsWindow, SB_VERT, &vertical, TRUE);
        GetClientRect(g_settingsWindow, &bounds);
        horizontal.nPage = bounds.right;
        vertical.nPage = bounds.bottom;
        SetScrollInfo(g_settingsWindow, SB_HORZ, &horizontal, TRUE);
        SetScrollInfo(g_settingsWindow, SB_VERT, &vertical, TRUE);
        scrollX = GetScrollPos(g_settingsWindow, SB_HORZ);
        scrollY = GetScrollPos(g_settingsWindow, SB_VERT);
        LayoutControls();
        InvalidateRect(g_settingsWindow, nullptr, TRUE);
        updating = false;
    }

    void ScrollPanel(int bar, int command)
    {
        SCROLLINFO info = {sizeof(info), SIF_ALL};
        GetScrollInfo(g_settingsWindow, bar, &info);
        int position = info.nPos;
        const int line = Scale(42, GetDpiForWindow(g_settingsWindow));
        switch (command)
        {
        case SB_LINEUP:
            position -= line;
            break;
        case SB_LINEDOWN:
            position += line;
            break;
        case SB_PAGEUP:
            position -= info.nPage;
            break;
        case SB_PAGEDOWN:
            position += info.nPage;
            break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            position = info.nTrackPos;
            break;
        case SB_TOP:
            position = info.nMin;
            break;
        case SB_BOTTOM:
            position = info.nMax;
            break;
        }
        info.fMask = SIF_POS;
        info.nPos = position;
        SetScrollInfo(g_settingsWindow, bar, &info, TRUE);
        scrollX = GetScrollPos(g_settingsWindow, SB_HORZ);
        scrollY = GetScrollPos(g_settingsWindow, SB_VERT);
        LayoutControls();
        RedrawWindow(g_settingsWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
    }

    void UpdateFonts()
    {
        const UINT dpi = GetDpiForWindow(g_settingsWindow);
        HFONT previousNormal = normalFont;
        HFONT previousTitle = titleFont;
        HFONT previousSmall = smallFont;
        normalFont = CreateFontW(-Scale(14, dpi), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        titleFont = CreateFontW(-Scale(26, dpi), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        smallFont = CreateFontW(-Scale(12, dpi), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        for (const auto& layout : layouts)
        {
            HFONT font = normalFont;
            if (layout.id == TITLE)
            {
                font = titleFont;
            }
            SendDlgItemMessageW(g_settingsWindow, layout.id, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        }
        DeleteObject(previousNormal);
        DeleteObject(previousTitle);
        DeleteObject(previousSmall);
    }

    bool CreateControls(HWND hwnd)
    {
        for (const auto& layout : layouts)
        {
            const wchar_t* className = L"BUTTON";
            DWORD style = WS_CHILD | WS_TABSTOP | BS_OWNERDRAW;
            DWORD extended = 0;
            if (layout.id >= TITLE)
            {
                className = L"STATIC";
                style = WS_CHILD | SS_LEFT | SS_NOPREFIX;
            }
            else if (layout.id == LANGUAGE)
            {
                className = L"COMBOBOX";
                style = WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL;
            }
            else if (layout.id == BUTTON_LIST)
            {
                className = L"LISTBOX";
                style = WS_CHILD | WS_TABSTOP | LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | WS_VSCROLL;
            }
            if (!CreateWindowExW(extended, className, L"", style, 0, 0, 1, 1, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(layout.id)), GetModuleHandleW(nullptr), nullptr))
            {
                return false;
            }
        }
        for (int index = 0; index < static_cast<int>(Language::Count); ++index)
        {
            SendDlgItemMessageW(hwnd, LANGUAGE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(LanguageName(static_cast<Language>(index))));
        }
        SendDlgItemMessageW(hwnd, LANGUAGE, CB_SETCURSEL, static_cast<WPARAM>(draft.language), 0);
        return true;
    }

    void DrawItem(const DRAWITEMSTRUCT& item)
    {
        RECT bounds = item.rcItem;
        const UINT dpi = GetDpiForWindow(g_settingsWindow);
        if (item.CtlID == LANGUAGE)
        {
            Fill(item.hDC, bounds, COLOR_CARD);
            if (item.itemState & ODS_SELECTED)
            {
                Fill(item.hDC, bounds, COLOR_HOVER);
            }
            bounds.left += Scale(12, dpi);
            Language language = draft.language;
            if (item.itemID != static_cast<UINT>(-1))
            {
                language = static_cast<Language>(item.itemID);
            }
            Label(item.hDC, bounds, LanguageName(language), normalFont, COLOR_TEXT);
            return;
        }
        if (item.CtlID == BUTTON_LIST)
        {
            Fill(item.hDC, bounds, COLOR_CARD);
            if (item.itemID >= draft.buttons.size())
            {
                return;
            }
            if (item.itemState & ODS_SELECTED)
            {
                Fill(item.hDC, bounds, COLOR_HOVER);
            }
            const auto& button = draft.buttons[item.itemID];
            RECT mark = bounds;
            mark.left += Scale(12, dpi);
            mark.right = mark.left + Scale(26, dpi);
            const wchar_t* symbol = L"\u2014";
            COLORREF color = COLOR_MUTED;
            if (button.enabled)
            {
                symbol = L"\u2713";
                color = COLOR_PRIMARY;
            }
            Label(item.hDC, mark, symbol, normalFont, color);
            bounds.left += Scale(46, dpi);
            bounds.right -= Scale(8, dpi);
            Label(item.hDC, bounds, Local(ButtonLabel(button.id)), normalFont, COLOR_TEXT, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
            if (item.itemState & ODS_FOCUS)
            {
                RECT focus = item.rcItem;
                InflateRect(&focus, -2, -2);
                DrawFocusRect(item.hDC, &focus);
            }
            return;
        }

        COLORREF background = COLOR_CARD;
        COLORREF foreground = COLOR_TEXT;
        if (item.CtlID == SAVE || item.CtlID == WEBSITE)
        {
            background = COLOR_PRIMARY;
            foreground = COLOR_BG;
        }
        if (item.CtlID >= NAV_GENERAL && item.CtlID <= NAV_ABOUT && static_cast<int>(item.CtlID) - NAV_GENERAL == selectedPage)
        {
            background = COLOR_HOVER;
            foreground = COLOR_PRIMARY;
        }
        if (item.itemState & ODS_SELECTED)
        {
            background = COLOR_BORDER;
            foreground = COLOR_TEXT;
        }
        if (item.itemState & ODS_DISABLED)
        {
            foreground = COLOR_MUTED;
        }
        Fill(item.hDC, bounds, background);
        if (item.CtlID == STARTUP || item.CtlID == START_HIDDEN)
        {
            bool checked = draftStartup;
            if (item.CtlID == START_HIDDEN)
            {
                checked = draft.startHidden;
            }
            RECT toggle = bounds;
            toggle.left += Scale(14, dpi);
            toggle.top += Scale(13, dpi);
            toggle.right = toggle.left + Scale(22, dpi);
            toggle.bottom = toggle.top + Scale(22, dpi);
            Fill(item.hDC, toggle, COLOR_BG);
            if (checked)
            {
                Fill(item.hDC, toggle, COLOR_PRIMARY);
                Label(item.hDC, toggle, L"\u2713", normalFont, COLOR_BG, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
            bounds.left += Scale(48, dpi);
        }
        wchar_t text[256] = {};
        GetWindowTextW(item.hwndItem, text, 256);
        UINT format = DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX;
        if (item.CtlID == STARTUP || item.CtlID == START_HIDDEN)
        {
            format = DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX;
        }
        Label(item.hDC, bounds, text, normalFont, foreground, format);
        if (item.itemState & ODS_FOCUS)
        {
            RECT focus = item.rcItem;
            InflateRect(&focus, -3, -3);
            DrawFocusRect(item.hDC, &focus);
        }
    }

    void EditLayout(int command)
    {
        const int selection = static_cast<int>(SendDlgItemMessageW(g_settingsWindow, BUTTON_LIST, LB_GETCURSEL, 0, 0));
        if (selection < 0 || selection >= static_cast<int>(draft.buttons.size()))
        {
            return;
        }
        int next = selection;
        if (command == MOVE_UP && selection > 0)
        {
            next = selection - 1;
            std::swap(draft.buttons[selection], draft.buttons[next]);
        }
        else if (command == MOVE_DOWN && selection < static_cast<int>(draft.buttons.size()) - 1)
        {
            next = selection + 1;
            std::swap(draft.buttons[selection], draft.buttons[next]);
        }
        else if (command == TOGGLE)
        {
            draft.buttons[selection].enabled = !draft.buttons[selection].enabled;
        }
        else if (command == RESET)
        {
            draft.buttons = AppSettings().buttons;
            next = 0;
        }
        PopulateButtons(next);
        ClearStatus();
    }

    void HandleCommand(int id, int notification)
    {
        if (id >= NAV_GENERAL && id <= NAV_ABOUT)
        {
            selectedPage = id - NAV_GENERAL;
            LayoutControls();
            RefreshText();
            return;
        }
        if (id == LANGUAGE && notification == CBN_SELCHANGE)
        {
            draft.language = static_cast<Language>(SendDlgItemMessageW(g_settingsWindow, LANGUAGE, CB_GETCURSEL, 0, 0));
            RefreshText();
            ClearStatus();
            return;
        }
        if (id == STARTUP)
        {
            draftStartup = !draftStartup;
            InvalidateRect(GetDlgItem(g_settingsWindow, STARTUP), nullptr, FALSE);
            ClearStatus();
            return;
        }
        if (id == START_HIDDEN)
        {
            draft.startHidden = !draft.startHidden;
            InvalidateRect(GetDlgItem(g_settingsWindow, START_HIDDEN), nullptr, FALSE);
            ClearStatus();
            return;
        }
        if (id >= MOVE_UP && id <= RESET)
        {
            EditLayout(id);
            return;
        }
        if (id == BUTTON_LIST && notification == LBN_SELCHANGE)
        {
            const LRESULT selection = SendDlgItemMessageW(g_settingsWindow, BUTTON_LIST, LB_GETCURSEL, 0, 0);
            EnableWindow(GetDlgItem(g_settingsWindow, MOVE_UP), selection > 0);
            EnableWindow(GetDlgItem(g_settingsWindow, MOVE_DOWN), selection >= 0 && selection < static_cast<int>(draft.buttons.size()) - 1);
            UpdateToolHint(static_cast<int>(selection));
            return;
        }
        if (id == BUTTON_LIST && notification == LBN_DBLCLK)
        {
            EditLayout(TOGGLE);
            return;
        }
        if (id == SAVE || id == IDOK)
        {
            if (!SaveSettings(draft, draftStartup))
            {
                MessageBoxW(g_settingsWindow, Local(Text::SaveFailed), L"PowerToolbar", MB_OK | MB_ICONWARNING);
                return;
            }
            LayoutToolbar(g_toolbar);
            LayoutToolbar(g_minimizedToolbar);
            HideButtonTooltip();
            SetWindowTextW(GetDlgItem(g_settingsWindow, STATUS), Local(Text::Saved));
            return;
        }
        if (id == CANCEL || id == IDCANCEL)
        {
            DestroyWindow(g_settingsWindow);
            return;
        }
        if (id == WEBSITE)
        {
            OpenProductWebsite(g_settingsWindow);
        }
    }

    LRESULT CALLBACK SettingsProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_CREATE:
            g_settingsWindow = hwnd;
            if (!CreateControls(hwnd))
            {
                return -1;
            }
            UpdateFonts();
            LayoutControls();
            PopulateButtons(0);
            RefreshText();
            return 0;
        case WM_COMMAND:
            HandleCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_SIZE:
            UpdateScrollbars();
            return 0;
        case WM_VSCROLL:
            ScrollPanel(SB_VERT, LOWORD(wParam));
            return 0;
        case WM_HSCROLL:
            ScrollPanel(SB_HORZ, LOWORD(wParam));
            return 0;
        case WM_MOUSEWHEEL:
            if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
            {
                ScrollPanel(SB_VERT, SB_LINEUP);
            }
            else
            {
                ScrollPanel(SB_VERT, SB_LINEDOWN);
            }
            return 0;
        case WM_DPICHANGED:
        {
            const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
            SetWindowPos(hwnd, nullptr, suggested->left, suggested->top, suggested->right - suggested->left, suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
            UpdateFonts();
            UpdateScrollbars();
            return 0;
        }
        case WM_CTLCOLORSTATIC:
        {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetBkColor(hdc, COLOR_BG);
            SetTextColor(hdc, COLOR_MUTED);
            if (GetDlgCtrlID(reinterpret_cast<HWND>(lParam)) == TITLE)
            {
                SetTextColor(hdc, COLOR_TEXT);
            }
            return reinterpret_cast<LRESULT>(backgroundBrush);
        }
        case WM_CTLCOLORLISTBOX:
            SetBkColor(reinterpret_cast<HDC>(wParam), COLOR_CARD);
            SetTextColor(reinterpret_cast<HDC>(wParam), COLOR_TEXT);
            return reinterpret_cast<LRESULT>(cardBrush);
        case WM_MEASUREITEM:
            reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->itemHeight = Scale(34, GetDpiForWindow(hwnd));
            return TRUE;
        case WM_DRAWITEM:
            DrawItem(*reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
            return TRUE;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT:
        {
            PAINTSTRUCT paint = {};
            HDC hdc = BeginPaint(hwnd, &paint);
            RECT bounds = {};
            GetClientRect(hwnd, &bounds);
            Fill(hdc, bounds, COLOR_BG);
            const UINT dpi = GetDpiForWindow(hwnd);
            SetViewportOrgEx(hdc, -scrollX, -scrollY, nullptr);
            RECT sidebar = { 0, 0, Scale(220, dpi), (std::max)(bounds.bottom + scrollY, static_cast<LONG>(Scale(650, dpi))) };
            Fill(hdc, sidebar, RGB(20, 26, 17));
            HICON icon = static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_MAINICON), IMAGE_ICON, Scale(44, dpi), Scale(44, dpi), LR_SHARED));
            DrawIconEx(hdc, Scale(24, dpi), Scale(32, dpi), icon, Scale(44, dpi), Scale(44, dpi), 0, nullptr, DI_NORMAL);
            RECT brand = { Scale(24, dpi), Scale(84, dpi), Scale(208, dpi), Scale(114, dpi) };
            Label(hdc, brand, L"PowerToolbar", normalFont, COLOR_TEXT);
            RECT author = { Scale(24, dpi), Scale(114, dpi), Scale(208, dpi), Scale(138, dpi) };
            Label(hdc, author, L"by Axinomyus", smallFont, COLOR_MUTED);
            RECT footer = { Scale(24, dpi), Scale(591, dpi), Scale(206, dpi), Scale(622, dpi) };
            Label(hdc, footer, L"AXINOMYUS  /  WINDOWS", smallFont, COLOR_MUTED);
            EndPaint(hwnd, &paint);
            return 0;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            g_settingsWindow = nullptr;
            DeleteObject(normalFont);
            DeleteObject(titleFont);
            DeleteObject(smallFont);
            normalFont = nullptr;
            titleFont = nullptr;
            smallFont = nullptr;
            UpdateToolbarPosition();
            return 0;
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

void OpenProductWebsite(HWND owner)
{
    if (reinterpret_cast<INT_PTR>(ShellExecuteW(owner, L"open", L"https://www.axinomyus.com/products/powertoolbar", nullptr, nullptr, SW_SHOWNORMAL)) <= 32)
    {
        MessageBoxW(owner, Tr(Text::WebsiteFailed), L"PowerToolbar", MB_OK | MB_ICONWARNING);
    }
}

void ShowSettingsWindow()
{
    if (g_settingsWindow)
    {
        ShowWindow(g_settingsWindow, SW_RESTORE);
        SetForegroundWindow(g_settingsWindow);
        return;
    }

    draft = GetSettings();
    draftStartup = StartsWithWindows();
    selectedPage = 0;
    scrollX = 0;
    scrollY = 0;
    HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSEXW existing = { sizeof(existing) };
    if (!GetClassInfoExW(instance, L"PowerToolbar.Settings", &existing))
    {
        backgroundBrush = CreateSolidBrush(COLOR_BG);
        cardBrush = CreateSolidBrush(COLOR_CARD);
        WNDCLASSEXW windowClass = { sizeof(windowClass) };
        windowClass.hInstance = instance;
        windowClass.lpfnWndProc = SettingsProc;
        windowClass.lpszClassName = L"PowerToolbar.Settings";
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_MAINICON));
        windowClass.hbrBackground = backgroundBrush;
        if (!RegisterClassExW(&windowClass))
        {
            MessageBoxW(g_toolbar, Tr(Text::OperationFailed), L"PowerToolbar", MB_OK | MB_ICONERROR);
            return;
        }
    }

    const UINT dpi = GetDpiForWindow(g_toolbar);
    RECT bounds = { 0, 0, Scale(890, dpi), Scale(650, dpi) };
    constexpr DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
    AdjustWindowRectExForDpi(&bounds, style, FALSE, WS_EX_APPWINDOW, dpi);
    MONITORINFO monitor = { sizeof(monitor) };
    GetMonitorInfoW(MonitorFromWindow(g_active, MONITOR_DEFAULTTONEAREST), &monitor);
    const int width  = (std::min)(bounds.right - bounds.left, monitor.rcWork.right - monitor.rcWork.left - 24);
    const int height = (std::min)(bounds.bottom - bounds.top, monitor.rcWork.bottom - monitor.rcWork.top - 24);
    const int x = monitor.rcWork.left + (monitor.rcWork.right - monitor.rcWork.left - width) / 2;
    const int y = (std::max)(monitor.rcWork.top, monitor.rcWork.top + (monitor.rcWork.bottom - monitor.rcWork.top - height) / 2);
    HWND window = CreateWindowExW(WS_EX_APPWINDOW | WS_EX_CONTROLPARENT, L"PowerToolbar.Settings", Tr(Text::SettingsTitle), style, x, y, width, height, nullptr, nullptr, instance, nullptr);
    if (!window)
    {
        MessageBoxW(g_toolbar, Tr(Text::OperationFailed), L"PowerToolbar", MB_OK | MB_ICONERROR);
        return;
    }
    BOOL dark = TRUE;
    DwmSetWindowAttribute(window, 20, &dark, sizeof(dark));
    ShowWindow(g_toolbar, SW_HIDE);
    ShowWindow(g_minimizedToolbar, SW_HIDE);
    HideButtonTooltip();
    ShowWindow(window, SW_SHOWNORMAL);
    SetForegroundWindow(window);
}
