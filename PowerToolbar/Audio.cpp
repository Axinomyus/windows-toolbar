#include "Audio.h"
#include "Globals.h"
#include "Localization.h"
#include "WindowOperations.h"
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <audioclient.h>
#include <tlhelp32.h>
#include <wrl/client.h>
#include <set>
#include <vector>

#pragma comment(lib, "ole32.lib")

namespace
{
    using Microsoft::WRL::ComPtr;

    struct AudioSession
    {
        ComPtr<ISimpleAudioVolume> volume;
        BOOL muted;
    };

    void AudioMessage(Text text)
    {
        const bool previousMenuState = g_menuOpen;
        g_menuOpen = true;
        MessageBoxW(g_toolbar, Tr(text), L"PowerToolbar", MB_OK | MB_ICONINFORMATION);
        g_menuOpen = previousMenuState;
    }

    std::set<DWORD> AppProcesses(DWORD target)
    {
        std::set<DWORD> processes = { target };
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
        {
            OutputDebugStringW(L"PowerToolbar: audio process enumeration failed.\n");
            return processes;
        }
        std::vector<PROCESSENTRY32W> entries;
        PROCESSENTRY32W entry = { sizeof(entry) };
        if (Process32FirstW(snapshot, &entry))
        {
            do
            {
                entries.push_back(entry);
            } while (Process32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);

        // Chromium and other multi-process apps play audio in child processes.
        bool changed = true;
        while (changed)
        {
            changed = false;
            for (const auto& process : entries)
            {
                if (processes.count(process.th32ParentProcessID) != 0 && processes.insert(process.th32ProcessID).second)
                {
                    changed = true;
                }
            }
        }
        return processes;
    }

    HRESULT CollectSessions(const std::set<DWORD>& processes, std::vector<AudioSession>& sessions)
    {
        ComPtr<IMMDeviceEnumerator> enumerator;
        HRESULT result = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&enumerator));
        if (FAILED(result))
        {
            return result;
        }
        ComPtr<IMMDeviceCollection> devices;
        result = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices);
        if (FAILED(result))
        {
            return result;
        }
        UINT count = 0;
        result = devices->GetCount(&count);
        if (FAILED(result))
        {
            return result;
        }
        for (UINT deviceIndex = 0; deviceIndex < count; ++deviceIndex)
        {
            ComPtr<IMMDevice> device;
            ComPtr<IAudioSessionManager2> manager;
            ComPtr<IAudioSessionEnumerator> sessionEnumerator;
            result = devices->Item(deviceIndex, &device);
            if (SUCCEEDED(result))
            {
                result = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_INPROC_SERVER, nullptr, reinterpret_cast<void**>(manager.GetAddressOf()));
            }
            if (SUCCEEDED(result))
            {
                result = manager->GetSessionEnumerator(&sessionEnumerator);
            }
            if (FAILED(result))
            {
                return result;
            }
            int sessionCount = 0;
            result = sessionEnumerator->GetCount(&sessionCount);
            if (FAILED(result))
            {
                return result;
            }
            for (int index = 0; index < sessionCount; ++index)
            {
                ComPtr<IAudioSessionControl> control;
                ComPtr<IAudioSessionControl2> details;
                result = sessionEnumerator->GetSession(index, &control);
                if (SUCCEEDED(result))
                {
                    result = control.As(&details);
                }
                if (FAILED(result))
                {
                    return result;
                }
                DWORD processId = 0;
                result = details->GetProcessId(&processId);
                // Multi-process shared sessions cannot be attributed safely to one app.
                if (result != S_OK || processes.count(processId) == 0)
                {
                    continue;
                }
                AudioSessionState state = AudioSessionStateExpired;
                result = control->GetState(&state);
                if (FAILED(result))
                {
                    return result;
                }
                if (state == AudioSessionStateExpired)
                {
                    continue;
                }
                AudioSession session = {};
                result = control.As(&session.volume);
                if (SUCCEEDED(result))
                {
                    result = session.volume->GetMute(&session.muted);
                }
                if (FAILED(result))
                {
                    return result;
                }
                sessions.push_back(std::move(session));
            }
        }
        return S_OK;
    }
}

AppAudioState GetAppAudioState(HWND hwnd)
{
    DWORD processId = 0;
    if (!IsManageableWindow(hwnd) || !GetWindowThreadProcessId(hwnd, &processId))
    {
        return AppAudioState::Unavailable;
    }
    std::vector<AudioSession> sessions;
    const HRESULT result = CollectSessions(AppProcesses(processId), sessions);
    static HRESULT previousResult = S_OK;
    if (FAILED(result) && result != previousResult)
    {
        wchar_t message[100] = {};
        swprintf_s(message, L"PowerToolbar: audio status query failed (0x%08lX).\n", static_cast<unsigned long>(result));
        OutputDebugStringW(message);
    }
    previousResult = result;
    if (FAILED(result) || sessions.empty())
    {
        return AppAudioState::Unavailable;
    }
    for (const auto& session : sessions)
    {
        if (!session.muted)
        {
            return AppAudioState::Audible;
        }
    }
    return AppAudioState::Muted;
}

void ToggleAppAudio(HWND hwnd)
{
    if (!IsManageableWindow(hwnd))
    {
        return;
    }
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    const auto processes = AppProcesses(processId);
    std::vector<AudioSession> sessions;
    HRESULT result = CollectSessions(processes, sessions);
    if (FAILED(result))
    {
        OutputDebugStringW(L"PowerToolbar: audio session collection failed.\n");
        AudioMessage(Text::AudioFailed);
        return;
    }
    if (sessions.empty())
    {
        AudioMessage(Text::NoAudio);
        return;
    }

    BOOL mute = FALSE;
    for (const auto& session : sessions)
    {
        if (!session.muted)
        {
            mute = TRUE;
            break;
        }
    }
    for (size_t index = 0; index < sessions.size(); ++index)
    {
        if (FAILED(sessions[index].volume->SetMute(mute, nullptr)))
        {
            for (size_t rollback = 0; rollback < index; ++rollback)
            {
                if (FAILED(sessions[rollback].volume->SetMute(sessions[rollback].muted, nullptr)))
                {
                    OutputDebugStringW(L"PowerToolbar: audio rollback failed for a disconnected session.\n");
                }
            }
            OutputDebugStringW(L"PowerToolbar: audio mute operation failed.\n");
            AudioMessage(Text::AudioFailed);
            return;
        }
    }
}
