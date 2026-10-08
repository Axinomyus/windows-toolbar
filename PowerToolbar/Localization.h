#pragma once
#include <cstddef>

enum class Language
{
    English,
    Turkish,
    German,
    Ukrainian,
    Russian,
    Count
};

enum class Text
{
    General, Toolbar, About, SettingsTitle, Tagline, LanguageLabel, Startup, StartHidden,
    GeneralHint, LayoutTitle, LayoutHint, MoveUp, MoveDown, ToggleButton, ResetLayout,
    Save, Cancel, Saved, Website, AboutBody, Version, License, Privacy,
    OpenToolbar, Compact, HideToolbar, Exit, RestoreWindow, Expand,
    Pin, Center, Fullscreen, Minimize, CloseWindow, EndApp, Audio,
    PinTip, CenterTip, FullscreenTip, MinimizeTip, CloseTip, EndTip, CompactTip, HideTip, AudioTip,
    Enabled, Disabled, AudioHint, FullscreenHint, SaveFailed, OperationFailed, PermissionHint,
    EndConfirm, ExplorerProtected, NoAudio, AudioFailed, TrayFailed, WebsiteFailed,
    StartupManaged, InitFailed, Screenshot, ScreenshotTip, ScreenshotFailed, MuteApp, UnmuteApp, ScreenshotHint,
    RevealToolbar, CollapseToolbar, Count
};

const wchar_t* Translate(Text text, Language language);
const wchar_t* Tr(Text text);
const wchar_t* LanguageName(Language language);
Language DetectLanguage();
