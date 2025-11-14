#include "Toolbar.h"
#include "Globals.h"
#include "Constants.h"
#include "WindowOperations.h"
#include "Drawing.h"
#include <dwmapi.h>
#include <cmath>
#include <algorithm>

double GetEaseOutCubic(double t) 
{
	return 1 - pow(1 - t, 3);
}

double GetAnimationProgress() 
{
	ULONGLONG currentTime	= GetTickCount64();
	ULONGLONG elapsed		= currentTime - g_creationTime;
	double progress			= min(1.0, (double)elapsed / ANIMATION_DURATION);

	return GetEaseOutCubic(progress);
}

void UpdateMinimizedToolbarPosition() 
{
	if (!g_minimizedToolbar || !g_active || !g_toolbarMinimized) 
	{
		if (g_minimizedToolbar && !g_toolbarMinimized) 
		{
			ShowWindow(g_minimizedToolbar, SW_HIDE);
		}

		return;
	}

	RECT rc;
	if (SUCCEEDED(DwmGetWindowAttribute(g_active, DWMWA_EXTENDED_FRAME_BOUNDS, &rc, sizeof(rc)))) 
	{
		int x = rc.left + 12;
		int y = max(rc.top - TOOLBAR_HEIGHT - GAP, 0);

		int minimizedWidth = 2 * (BUTTON_WIDTH + 12) + 24;

		RECT currentRect;
		GetWindowRect(g_minimizedToolbar, &currentRect);
		if (currentRect.left == x && currentRect.top == y && currentRect.right - currentRect.left == minimizedWidth) 
		{
			if (!IsWindowVisible(g_minimizedToolbar)) 
			{
				ShowWindow(g_minimizedToolbar, SW_SHOWNOACTIVATE);
			}

			return;
		}

		SetWindowPos(g_minimizedToolbar, HWND_TOPMOST, x, y, minimizedWidth, TOOLBAR_HEIGHT, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOREDRAW);

		ApplyModernRoundRegion(g_minimizedToolbar);
		RedrawWindow(g_minimizedToolbar, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
	}
}

void UpdateToolbarPosition() 
{
	HWND active = GetForegroundWindow();
	if (!active || active == g_toolbar || active == g_minimizedToolbar || IsSystemUIWindow(active)) 
	{
		if (g_visible) 
		{
			ShowWindow(g_toolbar, SW_HIDE);
			if (g_minimizedToolbar) 
			{
				ShowWindow(g_minimizedToolbar, SW_HIDE);
			}
			g_visible = false;
		}
		return;
	}

	bool windowChanged = (g_active != active);
	g_active = active;

	if (!g_toolbarMinimized && g_minimizedToolbar) 
	{
		ShowWindow(g_minimizedToolbar, SW_HIDE);
	}

	if (g_toolbarMinimized) 
	{
		UpdateMinimizedToolbarPosition();
		return;
	}

	RECT rc;
	if (SUCCEEDED(DwmGetWindowAttribute(active, DWMWA_EXTENDED_FRAME_BOUNDS, &rc, sizeof(rc)))) 
	{
		int width		= rc.right - rc.left;
		g_toolbarWidth	= (int)g_buttons.size() * (BUTTON_WIDTH + 12) + 24;

		int newX = rc.left + (width - g_toolbarWidth) / 2;
		int newY = rc.top - TOOLBAR_HEIGHT - GAP;

		if (!g_visible) 
		{
			g_creationTime = GetTickCount64();
			g_visible = true;
		}

		RECT currentRect;
		GetWindowRect(g_toolbar, &currentRect);
		if (currentRect.left == newX && currentRect.top == newY && currentRect.right - currentRect.left == g_toolbarWidth && !windowChanged) 
		{
			return;
		}

		double progress = GetAnimationProgress();
		int animOffset = (int)((1.0 - progress) * 20);

		SetWindowPos(g_toolbar, HWND_TOPMOST,
			newX,
			newY + animOffset,
			g_toolbarWidth, TOOLBAR_HEIGHT,
			SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOREDRAW);

		ApplyModernRoundRegion(g_toolbar);
		if (progress >= 1.0 || progress <= 0.1) 
		{
			RedrawWindow(g_toolbar, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		}
	}
}

void ShowToolbar(bool show) 
{
	if (show) 
	{
		g_toolbarMinimized = false;
		ShowWindow(g_toolbar, SW_SHOWNOACTIVATE);
		if (g_minimizedToolbar) 
		{
			ShowWindow(g_minimizedToolbar, SW_HIDE);
		}
		extern void RemoveTrayIcon();
		RemoveTrayIcon();
		UpdateToolbarPosition();
	}
	else 
	{
		g_toolbarMinimized = true;
		ShowWindow(g_toolbar, SW_HIDE);

		if (g_minimizedToolbar) 
		{
			UpdateMinimizedToolbarPosition();
		}
	}
}