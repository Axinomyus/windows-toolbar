#include "WindowOperations.h"
#include "Globals.h"
#include "Localization.h"
#include <tlhelp32.h>
#include <dwmapi.h>
#include <map>
#include <vector>

namespace
{
    constexpr const wchar_t* FULLSCREEN_PROPERTY = L"Axinomyus.PowerToolbar.Fullscreen";

    struct FullscreenState
    {
        LONG_PTR style;
        WINDOWPLACEMENT placement;
    };

    std::map<HWND, FullscreenState> fullscreenWindows;

    std::wstring GetProcessPath(HANDLE process)
    {
        std::wstring path(32768, L'\0');
        DWORD length = static_cast<DWORD>(path.size());
        if (!QueryFullProcessImageNameW(process, 0, path.data(), &length))
        {
            return {};
        }
        path.resize(length);
        return path;
    }

    void ReportFailure(const wchar_t* operation, DWORD error = GetLastError())
    {
        const std::wstring diagnostic = std::wstring(L"PowerToolbar: ") + operation + L" failed (" + std::to_wstring(error) + L").\n";
        OutputDebugStringW(diagnostic.c_str());
        const std::wstring message = std::wstring(Tr(Text::OperationFailed)) + L"\n\n" + Tr(Text::PermissionHint);
        const bool previousMenuState = g_menuOpen;
        g_menuOpen = true;
        MessageBoxW(g_toolbar, message.c_str(), L"PowerToolbar", MB_OK | MB_ICONWARNING);
        g_menuOpen = previousMenuState;
    }

    bool SetStyle(HWND hwnd, LONG_PTR style)
    {
        SetLastError(ERROR_SUCCESS);
        const LONG_PTR previous = SetWindowLongPtrW(hwnd, GWL_STYLE, style);
        return previous != 0 || GetLastError() == ERROR_SUCCESS;
    }

    bool RestoreFullscreen(HWND hwnd, const FullscreenState& state)
    {
        if (!SetStyle(hwnd, state.style))
        {
            return false;
        }

        WINDOWPLACEMENT placement = state.placement;
        if (!SetWindowPlacement(hwnd, &placement))
        {
            return false;
        }

        return SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED) != FALSE;
    }

    void PruneFullscreenWindows()
    {
        for (auto entry = fullscreenWindows.begin(); entry != fullscreenWindows.end();)
        {
            if (GetPropW(entry->first, FULLSCREEN_PROPERTY) != &entry->second)
            {
                entry = fullscreenWindows.erase(entry);
            }
            else
            {
                ++entry;
            }
        }
    }
}

std::wstring GetProcessNameFromHWND(HWND hwnd)
{
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!process)
    {
        return {};
    }

    const std::wstring path = GetProcessPath(process);
    CloseHandle(process);
    const size_t separator = path.find_last_of(L"\\/");
    if (separator != std::wstring::npos)
    {
        return path.substr(separator + 1);
    }
    return path;
}

void CloseActiveWindow(HWND hwnd)
{
    if (!IsManageableWindow(hwnd))
    {
        return;
    }

    // Never block the toolbar on a hung application's message loop.
    if (!PostMessageW(hwnd, WM_CLOSE, 0, 0))
    {
        ReportFailure(L"close this window");
    }
}

void CloseActiveProcess(HWND hwnd)
{
    if (!IsManageableWindow(hwnd))
    {
        return;
    }

    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
    if (!process)
    {
        ReportFailure(L"end this process");
        return;
    }

    if (!TerminateProcess(process, 1))
    {
        const DWORD error = GetLastError();
        CloseHandle(process);
        ReportFailure(L"end this process", error);
        return;
    }
    CloseHandle(process);
}

void ToggleFullscreen(HWND hwnd)
{
    if (!IsManageableWindow(hwnd))
    {
        return;
    }

    PruneFullscreenWindows();
    auto existing = fullscreenWindows.find(hwnd);
    if (existing != fullscreenWindows.end())
    {
        if (!RestoreFullscreen(hwnd, existing->second))
        {
            ReportFailure(L"restore this window");
            return;
        }
        RemovePropW(hwnd, FULLSCREEN_PROPERTY);
        fullscreenWindows.erase(existing);
        return;
    }

    FullscreenState state = {};
    state.style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    state.placement.length = sizeof(WINDOWPLACEMENT);
    MONITORINFO monitor = { sizeof(monitor) };
    if (!GetWindowPlacement(hwnd, &state.placement) || !GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor))
    {
        ReportFailure(L"read this window's position");
        return;
    }

    auto inserted = fullscreenWindows.emplace(hwnd, state).first;
    if (!SetPropW(hwnd, FULLSCREEN_PROPERTY, &inserted->second))
    {
        fullscreenWindows.erase(inserted);
        ReportFailure(L"save this window's fullscreen state");
        return;
    }

    if (!SetStyle(hwnd, state.style & ~(WS_OVERLAPPEDWINDOW | WS_MAXIMIZE)) ||
        !SetWindowPos(hwnd, nullptr, monitor.rcMonitor.left, monitor.rcMonitor.top, monitor.rcMonitor.right - monitor.rcMonitor.left, monitor.rcMonitor.bottom - monitor.rcMonitor.top, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED))
    {
        const DWORD error = GetLastError();
        if (RestoreFullscreen(hwnd, state))
        {
            RemovePropW(hwnd, FULLSCREEN_PROPERTY);
            fullscreenWindows.erase(inserted);
        }
        ReportFailure(L"make this window fullscreen", error);
    }
}

void RestoreFullscreenWindows()
{
    PruneFullscreenWindows();
    for (const auto& entry : fullscreenWindows)
    {
        if (!RestoreFullscreen(entry.first, entry.second))
        {
            ReportFailure(L"restore a fullscreen window");
        }
        RemovePropW(entry.first, FULLSCREEN_PROPERTY);
    }
    fullscreenWindows.clear();
}

bool HasFullscreenState(HWND hwnd)
{
    PruneFullscreenWindows();
    return fullscreenWindows.find(hwnd) != fullscreenWindows.end();
}

bool IsFullscreenWindow(HWND hwnd)
{
    RECT bounds = {};
    MONITORINFO monitor = { sizeof(monitor) };
    if (!GetWindowRect(hwnd, &bounds) || !GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor))
    {
        return false;
    }
    return bounds.left <= monitor.rcMonitor.left && bounds.top <= monitor.rcMonitor.top &&
        bounds.right >= monitor.rcMonitor.right && bounds.bottom >= monitor.rcMonitor.bottom;
}

void ToggleAlwaysOnTop(HWND hwnd)
{
    if (!IsManageableWindow(hwnd))
    {
        return;
    }

    HWND position = HWND_TOPMOST;
    if (GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
    {
        position = HWND_NOTOPMOST;
    }

    if (!SetWindowPos(hwnd, position, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
    {
        ReportFailure(L"change the pin state");
    }
}

void MinimizeWindow(HWND hwnd)
{
    if (!IsManageableWindow(hwnd))
    {
        return;
    }

    if (!ShowWindowAsync(hwnd, SW_MINIMIZE))
    {
        ReportFailure(L"minimize this window");
    }
}

void CenterWindow(HWND hwnd)
{
    if (!IsManageableWindow(hwnd))
    {
        return;
    }

    PruneFullscreenWindows();
    if (fullscreenWindows.find(hwnd) != fullscreenWindows.end())
    {
        ToggleFullscreen(hwnd);
        if (fullscreenWindows.find(hwnd) != fullscreenWindows.end())
        {
            return;
        }
    }
    if (IsZoomed(hwnd))
    {
        ShowWindow(hwnd, SW_RESTORE);
    }

    RECT bounds = {};
    MONITORINFO monitor = { sizeof(monitor) };
    if (!GetWindowRect(hwnd, &bounds) || !GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor))
    {
        ReportFailure(L"read this window's position");
        return;
    }

    const int width  = bounds.right - bounds.left;
    const int height = bounds.bottom - bounds.top;
    const int x = monitor.rcWork.left + (monitor.rcWork.right - monitor.rcWork.left - width) / 2;
    const int y = monitor.rcWork.top + (monitor.rcWork.bottom - monitor.rcWork.top - height) / 2;
    if (!SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE))
    {
        ReportFailure(L"center this window");
    }
}

void KillAllByProcessName(HWND hwnd)
{
    if (!IsManageableWindow(hwnd))
    {
        return;
    }

    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    HANDLE targetProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!targetProcess)
    {
        ReportFailure(L"identify this app");
        return;
    }

    const std::wstring targetPath = GetProcessPath(targetProcess);
    CloseHandle(targetProcess);
    const std::wstring name = GetProcessNameFromHWND(hwnd);
    if (targetPath.empty() || name.empty())
    {
        ReportFailure(L"identify this app");
        return;
    }
    if (_wcsicmp(name.c_str(), L"explorer.exe") == 0)
    {
        MessageBoxW(g_toolbar, Tr(Text::ExplorerProtected), L"PowerToolbar", MB_OK | MB_ICONINFORMATION);
        return;
    }

    const std::wstring message = name + L"\n\n" + Tr(Text::EndConfirm);
    const bool previousMenuState = g_menuOpen;
    g_menuOpen = true;
    const int answer = MessageBoxW(g_toolbar, message.c_str(), Tr(Text::EndApp), MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    g_menuOpen = previousMenuState;
    if (answer != IDYES)
    {
        return;
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        ReportFailure(L"list this app's processes");
        return;
    }

    DWORD failure = ERROR_SUCCESS;
    PROCESSENTRY32W entry = { sizeof(entry) };
    BOOL found = Process32FirstW(snapshot, &entry);
    while (found)
    {
        if (entry.th32ProcessID != GetCurrentProcessId() && _wcsicmp(entry.szExeFile, name.c_str()) == 0)
        {
            HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
            if (!process)
            {
                failure = GetLastError();
            }
            else
            {
                // Equal filenames in different folders must never target another app.
                const std::wstring path = GetProcessPath(process);
                if (path.empty())
                {
                    failure = GetLastError();
                }
                else if (_wcsicmp(path.c_str(), targetPath.c_str()) == 0 && !TerminateProcess(process, 1))
                {
                    failure = GetLastError();
                }
                CloseHandle(process);
            }
        }
        found = Process32NextW(snapshot, &entry);
    }
    const DWORD enumerationError = GetLastError();
    CloseHandle(snapshot);
    if (enumerationError != ERROR_NO_MORE_FILES)
    {
        failure = enumerationError;
    }
    if (failure != ERROR_SUCCESS)
    {
        ReportFailure(L"end all of this app's processes", failure);
    }
}

bool IsSystemUIWindow(HWND hwnd)
{
    if (!hwnd)
    {
        return true;
    }

    wchar_t className[256] = {};
    GetClassNameW(hwnd, className, 256);
    const wchar_t* excludedClasses[] =
    {
        L"Shell_TrayWnd", L"Shell_SecondaryTrayWnd", L"Progman", L"WorkerW",
        L"Start", L"DV2ControlHost", L"ImmersiveLauncher",
        L"Shell_InputSwitchTopLevelWindow", L"MultitaskingViewFrame", L"#32768"
    };
    for (const wchar_t* excluded : excludedClasses)
    {
        if (wcscmp(className, excluded) == 0)
        {
            return true;
        }
    }
    return false;
}

bool IsManageableWindow(HWND hwnd)
{
    if (!IsWindow(hwnd) || !IsWindowVisible(hwnd) || IsIconic(hwnd) || IsSystemUIWindow(hwnd))
    {
        return false;
    }

    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == GetCurrentProcessId())
    {
        return false;
    }
    if (GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)
    {
        return false;
    }

    DWORD cloaked = 0;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked != 0)
    {
        return false;
    }
    return true;
}
