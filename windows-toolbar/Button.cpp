#include "Button.h"
#include "Globals.h"
#include "Constants.h"
#include "Drawing.h"
#include "WindowOperations.h"
#include "Toolbar.h"
#include "SystemTray.h"
#include "Tooltip.h"
#include <windowsx.h>

LRESULT CALLBACK ModernButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	static bool hover = false, press = false;

	switch (msg) 
	{
	case WM_MOUSEMOVE: 
	{
		if (!hover) 
		{
			hover = true;

			TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };

			TrackMouseEvent(&tme);

			g_tooltipShowTime		= GetTickCount64();
			g_currentTooltipButton	= hwnd;
		}
		break;
	}

	case WM_MOUSELEAVE:
		hover = false;
		press = false;
		if (g_currentTooltipButton == hwnd) 
		{
			HideButtonTooltip();
		}
		break;

	case WM_LBUTTONDOWN:
		press = true;
		RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		if (g_currentTooltipButton == hwnd) 
		{
			HideButtonTooltip();
		}
		break;

	case WM_LBUTTONUP:
		press = false;
		RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		{
			int id		= GetDlgCtrlID(hwnd);
			HWND parent = GetParent(hwnd);

			switch (id) 
			{
			case 7: ShowToolbar(false); 
				break;
			case 8:
			{
				ShowWindow(g_toolbar, SW_HIDE);
				if (g_minimizedToolbar)
				{
					ShowWindow(g_minimizedToolbar, SW_HIDE);
				}
				AddTrayIcon(g_toolbar);
				break;
			}
			case 100: ShowToolbar(true); 
				break;
			case 101: 
			{
				ShowWindow(g_toolbar, SW_HIDE);
				if (g_minimizedToolbar) 
				{
					ShowWindow(g_minimizedToolbar, SW_HIDE);
				}
				AddTrayIcon(g_toolbar);
				break;
			}
			default:
				if (g_active && g_active != g_toolbar && g_active != g_minimizedToolbar) 
				{
					switch (id) 
					{
					case 1: CloseActiveWindow(g_active); break;
					case 2: ToggleFullscreen(g_active); break;
					case 3: MinimizeWindow(g_active); break;
					case 4: ToggleAlwaysOnTop(g_active); break;
					case 5: CenterWindow(g_active); break;
					case 6: KillAllByProcessName(g_active); break;
					}
				}
				break;
			}
		}
		break;

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		RECT rc; 
		GetClientRect(hwnd, &rc);
		int btnId = GetDlgCtrlID(hwnd);

		std::wstring text, icon;
		COLORREF color = COLOR_PRIMARY;

		if (btnId == 100) 
		{
			icon	= ICON_UP;
			color	= COLOR_PRIMARY;
		}
		else if (btnId == 101) 
		{
			icon	= ICON_CLOSE_TOOLBAR;
			color	= COLOR_RED;
		}
		else 
		{
			for (auto& b : g_buttons) 
			{
				if (b.id == btnId) 
				{
					text	= b.label;
					icon	= b.icon;
					color	= b.color;
					break;
				}
			}
		}

		DrawModernButton(hdc, rc, text, icon, color, hover, press);
		EndPaint(hwnd, &ps);
		break;
	}

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}