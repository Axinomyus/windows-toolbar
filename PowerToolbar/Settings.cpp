#include "Settings.h"
#include "Globals.h"
#include "Constants.h"
#include <string>
#include <vector>

#pragma comment(lib, "advapi32.lib")

namespace
{
    constexpr const wchar_t* SETTINGS_KEY = L"Software\\Axinomyus\\PowerToolbar";
    constexpr const wchar_t* STARTUP_KEY  = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    constexpr DWORD SETTINGS_FORMAT      = 2;

    struct StoredSettings
    {
        DWORD format;
        DWORD language;
        DWORD startHidden;
        DWORD order[10];
        DWORD enabledMask;
    };

    AppSettings currentSettings;

    bool HasStartupValue(HKEY root)
    {
        DWORD size = 0;
        return RegGetValueW(root, STARTUP_KEY, L"PowerToolbar", RRF_RT_REG_SZ, nullptr, nullptr, &size) == ERROR_SUCCESS && size > sizeof(wchar_t);
    }

    struct StartupValue
    {
        HKEY root;
        DWORD type = REG_SZ;
        std::vector<BYTE> data;
        bool exists = false;
    };

    bool ReadStartup(StartupValue& value)
    {
        DWORD size = 0;
        LSTATUS status = RegGetValueW(value.root, STARTUP_KEY, L"PowerToolbar", RRF_RT_ANY | RRF_NOEXPAND, &value.type, nullptr, &size);
        if (status == ERROR_FILE_NOT_FOUND || status == ERROR_PATH_NOT_FOUND)
        {
            return true;
        }
        if (status != ERROR_SUCCESS || size > 65536)
        {
            return false;
        }
        value.data.resize(size);
        status = RegGetValueW(value.root, STARTUP_KEY, L"PowerToolbar", RRF_RT_ANY | RRF_NOEXPAND, &value.type, value.data.data(), &size);
        value.exists = status == ERROR_SUCCESS;
        return value.exists;
    }

    bool WriteStartup(const StartupValue& value)
    {
        HKEY key = nullptr;
        LSTATUS status = RegCreateKeyExW(value.root, STARTUP_KEY, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr);
        if (status != ERROR_SUCCESS)
        {
            SetLastError(status);
            return false;
        }

        if (value.exists)
        {
            status = RegSetValueExW(key, L"PowerToolbar", 0, value.type, value.data.data(), static_cast<DWORD>(value.data.size()));
        }
        else
        {
            status = RegDeleteValueW(key, L"PowerToolbar");
            if (status == ERROR_FILE_NOT_FOUND)
            {
                status = ERROR_SUCCESS;
            }
        }
        RegCloseKey(key);
        SetLastError(status);
        return status == ERROR_SUCCESS;
    }

    bool RestoreStartup(const std::vector<StartupValue>& previous)
    {
        bool restored = true;
        for (const auto& value : previous)
        {
            if (!WriteStartup(value))
            {
                restored = false;
                OutputDebugStringW(L"PowerToolbar: startup rollback failed.\n");
            }
        }
        return restored;
    }

    bool SetStartup(bool enabled, std::vector<StartupValue>& previous)
    {
        for (HKEY root : {HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE})
        {
            StartupValue value = {root};
            if (!ReadStartup(value))
            {
                return false;
            }
            if (value.exists)
            {
                previous.push_back(std::move(value));
            }
        }
        if (enabled)
        {
            // New startup registrations are per-user, matching the default installer.
            previous.clear();
            StartupValue value = {HKEY_CURRENT_USER};
            if (!ReadStartup(value))
            {
                return false;
            }
            previous.push_back(value);
            std::wstring path(32768, L'\0');
            const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
            if (length == 0 || length >= path.size())
            {
                return false;
            }
            path.resize(length);
            const std::wstring command = L"\"" + path + L"\"";
            const BYTE* bytes = reinterpret_cast<const BYTE*>(command.c_str());
            value.data.assign(bytes, bytes + (command.size() + 1) * sizeof(wchar_t));
            value.type = REG_SZ;
            value.exists = true;
            return WriteStartup(value);
        }

        std::vector<StartupValue> changed;
        for (const auto& value : previous)
        {
            StartupValue removed = value;
            removed.exists = false;
            if (!WriteStartup(removed))
            {
                if (!RestoreStartup(changed))
                {
                    MessageBoxW(g_toolbar, Tr(Text::StartupManaged), L"PowerToolbar", MB_OK | MB_ICONWARNING);
                }
                return false;
            }
            changed.push_back(value);
        }
        return true;
    }

    bool ValidSettings(const AppSettings& settings)
    {
        if (settings.language < Language::English || settings.language >= Language::Count)
        {
            return false;
        }
        DWORD seen = 0;
        for (const auto& button : settings.buttons)
        {
            if (button.id < 1 || button.id > 10 || (seen & (1u << button.id)))
            {
                return false;
            }
            seen |= 1u << button.id;
        }
        return seen == 0x7fe;
    }
}

void LoadSettings()
{
    currentSettings.language = DetectLanguage();
    StoredSettings stored = {};
    DWORD size = sizeof(stored);
    const LSTATUS status = RegGetValueW(HKEY_CURRENT_USER, SETTINGS_KEY, L"Settings", RRF_RT_REG_BINARY, nullptr, &stored, &size);
    if (status == ERROR_FILE_NOT_FOUND || status == ERROR_PATH_NOT_FOUND)
    {
        return;
    }
    if (status == ERROR_SUCCESS && size == sizeof(stored) - sizeof(DWORD) && stored.format == 1)
    {
        // Preserve the nine-button layout when adding the optional screenshot tool.
        stored.enabledMask = stored.order[9];
        stored.order[9] = 10;
        stored.format = SETTINGS_FORMAT;
        size = sizeof(stored);
    }
    if (status != ERROR_SUCCESS || size != sizeof(stored) || stored.format != SETTINGS_FORMAT || stored.startHidden > 1 || (stored.enabledMask & ~0x7fe) != 0)
    {
        OutputDebugStringW(L"PowerToolbar: invalid or unreadable saved settings; using defaults.\n");
        return;
    }

    AppSettings loaded;
    loaded.language = static_cast<Language>(stored.language);
    loaded.startHidden = stored.startHidden != 0;
    for (size_t index = 0; index < loaded.buttons.size(); ++index)
    {
        const DWORD id = stored.order[index];
        if (id < 1 || id > 10)
        {
            OutputDebugStringW(L"PowerToolbar: invalid button ID in saved settings.\n");
            return;
        }
        loaded.buttons[index] = { static_cast<int>(id), (stored.enabledMask & (1u << id)) != 0 };
    }
    if (ValidSettings(loaded))
    {
        currentSettings = loaded;
    }
    else
    {
        OutputDebugStringW(L"PowerToolbar: saved settings validation failed.\n");
    }
}

const AppSettings& GetSettings()
{
    return currentSettings;
}

bool StartsWithWindows()
{
    return HasStartupValue(HKEY_CURRENT_USER) || HasStartupValue(HKEY_LOCAL_MACHINE);
}

bool SaveSettings(const AppSettings& settings, bool startWithWindows)
{
    if (!ValidSettings(settings))
    {
        return false;
    }

    HKEY key = nullptr;
    const LSTATUS opened = RegCreateKeyExW(HKEY_CURRENT_USER, SETTINGS_KEY, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr);
    if (opened != ERROR_SUCCESS)
    {
        OutputDebugStringW(L"PowerToolbar: cannot open settings for writing.\n");
        return false;
    }

    const bool previousStartup = StartsWithWindows();
    std::vector<StartupValue> previousValues;
    if (previousStartup != startWithWindows && !SetStartup(startWithWindows, previousValues))
    {
        RegCloseKey(key);
        OutputDebugStringW(L"PowerToolbar: could not update Windows startup.\n");
        return false;
    }

    StoredSettings stored = {};
    stored.format = SETTINGS_FORMAT;
    stored.language = static_cast<DWORD>(settings.language);
    stored.startHidden = settings.startHidden;
    for (size_t index = 0; index < settings.buttons.size(); ++index)
    {
        const auto& button = settings.buttons[index];
        stored.order[index] = button.id;
        if (button.enabled)
        {
            stored.enabledMask |= 1u << button.id;
        }
    }

    const LSTATUS status = RegSetValueExW(key, L"Settings", 0, REG_BINARY, reinterpret_cast<const BYTE*>(&stored), sizeof(stored));
    RegCloseKey(key);
    if (status != ERROR_SUCCESS)
    {
        if (previousStartup != startWithWindows && !RestoreStartup(previousValues))
        {
            OutputDebugStringW(L"PowerToolbar: settings save and startup rollback both failed.\n");
            MessageBoxW(g_toolbar, Tr(Text::StartupManaged), L"PowerToolbar", MB_OK | MB_ICONWARNING);
        }
        return false;
    }
    currentSettings = settings;
    RefreshToolbarButtons();
    return true;
}

Text ButtonLabel(int id)
{
    constexpr Text labels[] = { Text::Toolbar, Text::CloseWindow, Text::Fullscreen, Text::Minimize, Text::Pin, Text::Center, Text::EndApp, Text::Compact, Text::HideToolbar, Text::Audio, Text::Screenshot };
    if (id < 1 || id > 10)
    {
        return Text::Toolbar;
    }
    return labels[id];
}

Text ButtonTooltip(int id)
{
    constexpr Text labels[] = { Text::Toolbar, Text::CloseTip, Text::FullscreenTip, Text::MinimizeTip, Text::PinTip, Text::CenterTip, Text::EndTip, Text::CompactTip, Text::HideTip, Text::AudioTip, Text::ScreenshotTip };
    if (id < 1 || id > 10)
    {
        return Text::Toolbar;
    }
    return labels[id];
}

void RefreshToolbarButtons()
{
    g_buttons.clear();
    for (const auto& setting : currentSettings.buttons)
    {
        COLORREF color = COLOR_TEXT;
        if (setting.id == 4)
        {
            color = COLOR_PRIMARY;
        }
        else if (setting.id == 6)
        {
            color = COLOR_RED;
        }
        else if (setting.id == 7 || setting.id == 8)
        {
            color = COLOR_MUTED;
        }
        g_buttons.push_back({setting.id, Tr(ButtonLabel(setting.id)), color, Tr(ButtonTooltip(setting.id)), setting.enabled});
    }
}
