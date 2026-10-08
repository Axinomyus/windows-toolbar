#pragma once
#include <array>
#include <windows.h>
#include "Localization.h"

struct ButtonSetting
{
    int id;
    bool enabled;
};

struct AppSettings
{
    Language language = Language::English;
    bool startHidden = false;
    std::array<ButtonSetting, 10> buttons =
    {{
        {4, true}, {5, true}, {2, true}, {3, true}, {1, true},
        {6, true}, {7, true}, {8, true}, {9, false}, {10, false}
    }};
};

void LoadSettings();
const AppSettings& GetSettings();
bool SaveSettings(const AppSettings& settings, bool startWithWindows);
bool StartsWithWindows();
void RefreshToolbarButtons();
Text ButtonLabel(int id);
Text ButtonTooltip(int id);
