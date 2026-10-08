#pragma once
#include <windows.h>

enum class AppAudioState
{
    Unavailable,
    Audible,
    Muted
};

AppAudioState GetAppAudioState(HWND hwnd);
void ToggleAppAudio(HWND hwnd);
