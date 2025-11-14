#pragma once
#include <windows.h>
#include <string>

struct ModernButton {
	int id;
	std::wstring label;
	std::wstring icon;
	COLORREF color;
	std::wstring tooltip;
};