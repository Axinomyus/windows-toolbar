#pragma once
#include <windows.h>
#include <algorithm>

inline RECT CalculateTopCenteredBounds(const RECT& area, int width, int height, int offset)
{
    if (width <= 0 || height <= 0 || offset < 0 || width > area.right - area.left || height + offset > area.bottom - area.top)
    {
        return {};
    }
    const LONG left = area.left + (area.right - area.left - width) / 2;
    const LONG top  = area.top + offset;
    return {left, top, left + width, top + height};
}

inline RECT CalculateToolbarBounds(const RECT& target, const RECT& workArea, int width, int height, int gap)
{
    const int availableWidth  = workArea.right - workArea.left;
    const int availableHeight = workArea.bottom - workArea.top;
    if (width > availableWidth || height > availableHeight)
    {
        return {};
    }

    LONG x = target.left + (target.right - target.left - width) / 2;
    LONG y = target.top - height - gap;
    if (y < workArea.top || y + height > workArea.bottom)
    {
        return {};
    }

    x = (std::max)(workArea.left, (std::min)(x, workArea.right - width));
    return { x, y, x + width, y + height };
}
