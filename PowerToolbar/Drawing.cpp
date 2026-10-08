#include "Drawing.h"
#include "Constants.h"
#include "Globals.h"
#include "resource.h"
#include <gdiplus.h>

namespace
{
    Gdiplus::Color ToColor(COLORREF color)
    {
        return Gdiplus::Color(255, GetRValue(color), GetGValue(color), GetBValue(color));
    }

    void RoundedPath(Gdiplus::GraphicsPath& path, float x, float y, float width, float height, float radius)
    {
        const float diameter = radius * 2;
        path.AddArc(x, y, diameter, diameter, 180, 90);
        path.AddArc(x + width - diameter, y, diameter, diameter, 270, 90);
        path.AddArc(x + width - diameter, y + height - diameter, diameter, diameter, 0, 90);
        path.AddArc(x, y + height - diameter, diameter, diameter, 90, 90);
        path.CloseFigure();
    }

    void DrawGlyph(Gdiplus::Graphics& graphics, int id, Gdiplus::Pen& pen, bool active)
    {
        switch (id)
        {
        case 1:
            graphics.DrawLine(&pen, 6.0f, 6.0f, 18.0f, 18.0f);
            graphics.DrawLine(&pen, 18.0f, 6.0f, 6.0f, 18.0f);
            break;
        case 2:
            graphics.DrawLine(&pen, 4.0f, 9.0f, 4.0f, 4.0f);
            graphics.DrawLine(&pen, 4.0f, 4.0f, 9.0f, 4.0f);
            graphics.DrawLine(&pen, 15.0f, 4.0f, 20.0f, 4.0f);
            graphics.DrawLine(&pen, 20.0f, 4.0f, 20.0f, 9.0f);
            graphics.DrawLine(&pen, 20.0f, 15.0f, 20.0f, 20.0f);
            graphics.DrawLine(&pen, 20.0f, 20.0f, 15.0f, 20.0f);
            graphics.DrawLine(&pen, 9.0f, 20.0f, 4.0f, 20.0f);
            graphics.DrawLine(&pen, 4.0f, 20.0f, 4.0f, 15.0f);
            break;
        case 3:
            graphics.DrawLine(&pen, 5.0f, 16.0f, 19.0f, 16.0f);
            break;
        case 4:
        {
            Gdiplus::PointF points[] = { {8, 4}, {16, 4}, {15, 11}, {18, 15}, {6, 15}, {9, 11}, {8, 4} };
            graphics.DrawLines(&pen, points, 7);
            graphics.DrawLine(&pen, 12.0f, 15.0f, 12.0f, 21.0f);
            break;
        }
        case 5:
            graphics.DrawRectangle(&pen, 8.0f, 8.0f, 8.0f, 8.0f);
            graphics.DrawLine(&pen, 12.0f, 2.0f, 12.0f, 5.0f);
            graphics.DrawLine(&pen, 12.0f, 19.0f, 12.0f, 22.0f);
            graphics.DrawLine(&pen, 2.0f, 12.0f, 5.0f, 12.0f);
            graphics.DrawLine(&pen, 19.0f, 12.0f, 22.0f, 12.0f);
            break;
        case 6:
            graphics.DrawArc(&pen, 4.0f, 4.0f, 16.0f, 16.0f, -52.0f, 284.0f);
            graphics.DrawLine(&pen, 12.0f, 2.0f, 12.0f, 12.0f);
            break;
        case 7:
            graphics.DrawLine(&pen, 6.0f, 9.0f, 12.0f, 15.0f);
            graphics.DrawLine(&pen, 12.0f, 15.0f, 18.0f, 9.0f);
            break;
        case 100:
            graphics.DrawLine(&pen, 6.0f, 15.0f, 12.0f, 9.0f);
            graphics.DrawLine(&pen, 12.0f, 9.0f, 18.0f, 15.0f);
            break;
        case 8:
        case 101:
            graphics.DrawLine(&pen, 4.0f, 15.0f, 4.0f, 20.0f);
            graphics.DrawLine(&pen, 4.0f, 20.0f, 20.0f, 20.0f);
            graphics.DrawLine(&pen, 20.0f, 20.0f, 20.0f, 15.0f);
            graphics.DrawLine(&pen, 12.0f, 3.0f, 12.0f, 14.0f);
            graphics.DrawLine(&pen, 8.0f, 10.0f, 12.0f, 14.0f);
            graphics.DrawLine(&pen, 12.0f, 14.0f, 16.0f, 10.0f);
            break;
        case 9:
        {
            Gdiplus::PointF points[] = { {4, 9}, {8, 9}, {13, 5}, {13, 19}, {8, 15}, {4, 15}, {4, 9} };
            graphics.DrawLines(&pen, points, 7);
            if (active)
            {
                graphics.DrawLine(&pen, 17.0f, 9.0f, 22.0f, 15.0f);
                graphics.DrawLine(&pen, 22.0f, 9.0f, 17.0f, 15.0f);
            }
            else
            {
                graphics.DrawArc(&pen, 12.0f, 7.0f, 7.0f, 10.0f, -65.0f, 130.0f);
                graphics.DrawArc(&pen, 12.0f, 3.0f, 13.0f, 18.0f, -65.0f, 130.0f);
            }
            break;
        }
        case 10:
            graphics.DrawRectangle(&pen, 3.0f, 7.0f, 18.0f, 13.0f);
            graphics.DrawLine(&pen, 7.0f, 7.0f, 9.0f, 4.0f);
            graphics.DrawLine(&pen, 9.0f, 4.0f, 15.0f, 4.0f);
            graphics.DrawLine(&pen, 15.0f, 4.0f, 17.0f, 7.0f);
            graphics.DrawEllipse(&pen, 8.0f, 10.0f, 8.0f, 7.0f);
            break;
        }
    }

    void DrawLabel(HDC hdc, RECT bounds, const wchar_t* text, int size, int weight, COLORREF color, UINT dpi)
    {
        HFONT font = CreateFontW(-Scale(size, dpi), 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HGDIOBJ previousFont = SelectObject(hdc, font);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, color);
        DrawTextW(hdc, text, -1, &bounds, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
        SelectObject(hdc, previousFont);
        DeleteObject(font);
    }
}

void DrawModernButton(HDC hdc, RECT rc, int buttonId, COLORREF color, bool hover, bool press, bool active, UINT dpi)
{
    Gdiplus::Graphics graphics(hdc);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.Clear(ToColor(COLOR_BG));

    COLORREF background = COLOR_CARD;
    if (hover)
    {
        background = COLOR_HOVER;
    }
    if (active)
    {
        background = COLOR_PRIMARY;
        color = COLOR_BG;
    }
    if (press)
    {
        background = RGB(73, 91, 47);
        color = COLOR_TEXT;
    }

    Gdiplus::GraphicsPath path;
    RoundedPath(path, 1.0f, 1.0f, static_cast<float>(rc.right - 2), static_cast<float>(rc.bottom - 2), static_cast<float>(Scale(9, dpi)));
    Gdiplus::SolidBrush brush(ToColor(background));
    graphics.FillPath(&brush, &path);

    const float factor = static_cast<float>(dpi) / 96.0f;
    graphics.TranslateTransform((rc.right - 24 * factor) / 2, (rc.bottom - 24 * factor) / 2);
    graphics.ScaleTransform(factor, factor);
    Gdiplus::Pen pen(ToColor(color), 1.7f);
    pen.SetStartCap(Gdiplus::LineCapRound);
    pen.SetEndCap(Gdiplus::LineCapRound);
    pen.SetLineJoin(Gdiplus::LineJoinRound);
    DrawGlyph(graphics, buttonId, pen, active);
}

void ApplyModernRoundRegion(HWND hwnd)
{
    RECT rc = {};
    GetClientRect(hwnd, &rc);
    const int radius = Scale(CORNER_RADIUS * 2, GetDpiForWindow(hwnd));
    HRGN region = CreateRoundRectRgn(0, 0, rc.right + 1, rc.bottom + 1, radius, radius);
    if (!SetWindowRgn(hwnd, region, TRUE))
    {
        DeleteObject(region);
    }
    // On success Windows owns the region.
}

void PaintModernToolbar(HWND hwnd, HDC hdc)
{
    RECT rc = {};
    GetClientRect(hwnd, &rc);
    const UINT dpi = GetDpiForWindow(hwnd);
    {
        Gdiplus::Graphics graphics(hdc);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        graphics.Clear(ToColor(COLOR_BG));
        Gdiplus::GraphicsPath path;
        RoundedPath(path, 0.5f, 0.5f, static_cast<float>(rc.right - 1), static_cast<float>(rc.bottom - 1), static_cast<float>(Scale(CORNER_RADIUS, dpi)));
        Gdiplus::Pen border(ToColor(COLOR_BORDER), 1.0f);
        graphics.DrawPath(&border, &path);
        if (hwnd == g_toolbar)
        {
            graphics.DrawLine(&border, Scale(154, dpi), Scale(18, dpi), Scale(154, dpi), Scale(46, dpi));
        }
    }

    HICON icon = static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_MAINICON), IMAGE_ICON, Scale(32, dpi), Scale(32, dpi), LR_SHARED));
    DrawIconEx(hdc, Scale(12, dpi), Scale(16, dpi), icon, Scale(32, dpi), Scale(32, dpi), 0, nullptr, DI_NORMAL);
    if (hwnd == g_toolbar)
    {
        RECT title  = { Scale(52, dpi), Scale(14, dpi), Scale(151, dpi), Scale(34, dpi) };
        RECT author = { Scale(52, dpi), Scale(34, dpi), Scale(151, dpi), Scale(50, dpi) };
        DrawLabel(hdc, title, L"PowerToolbar", 13, FW_SEMIBOLD, COLOR_TEXT, dpi);
        DrawLabel(hdc, author, L"by Axinomyus", 10, FW_NORMAL, COLOR_MUTED, dpi);
    }
}
