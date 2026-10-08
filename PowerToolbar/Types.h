#pragma once
#include <windows.h>
#include <string>

enum class ToolbarMode
{
    Expanded,
    Compact,
    Hidden
};

struct ModernButton
{
    int id;
    std::wstring label;
    COLORREF color;
    std::wstring tooltip;
    bool enabled;
};
