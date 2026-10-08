#include "Screenshot.h"
#include "Globals.h"
#include "Localization.h"
#include "WindowOperations.h"
#include "Toolbar.h"
#include <dwmapi.h>
#include <gdiplus.h>
#include <shobjidl.h>
#include <wrl/client.h>
#include <string>
#include <vector>

namespace
{
    HBITMAP CaptureContent(HWND hwnd)
    {
        RECT client = {};
        POINT origin = {};
        if (!GetClientRect(hwnd, &client) || !ClientToScreen(hwnd, &origin))
        {
            return nullptr;
        }
        const int width  = client.right;
        const int height = client.bottom;
        const int left   = GetSystemMetrics(SM_XVIRTUALSCREEN);
        const int top    = GetSystemMetrics(SM_YVIRTUALSCREEN);
        if (width <= 0 || height <= 0 || origin.x < left || origin.y < top ||
            origin.x + width > left + GetSystemMetrics(SM_CXVIRTUALSCREEN) ||
            origin.y + height > top + GetSystemMetrics(SM_CYVIRTUALSCREEN))
        {
            return nullptr;
        }

        HDC screen = GetDC(nullptr);
        HDC memory = CreateCompatibleDC(screen);
        HBITMAP bitmap = CreateCompatibleBitmap(screen, width, height);
        if (!screen || !memory || !bitmap)
        {
            DeleteObject(bitmap);
            DeleteDC(memory);
            ReleaseDC(nullptr, screen);
            return nullptr;
        }
        HGDIOBJ previous = SelectObject(memory, bitmap);
        const BOOL copied = BitBlt(memory, 0, 0, width, height, screen, origin.x, origin.y, SRCCOPY | CAPTUREBLT);
        SelectObject(memory, previous);
        DeleteDC(memory);
        ReleaseDC(nullptr, screen);
        if (!copied)
        {
            DeleteObject(bitmap);
            return nullptr;
        }
        return bitmap;
    }

    bool WritePng(HBITMAP bitmap, const std::wstring& path)
    {
        UINT count = 0;
        UINT size = 0;
        if (Gdiplus::GetImageEncodersSize(&count, &size) != Gdiplus::Ok || size == 0)
        {
            return false;
        }
        std::vector<BYTE> storage(size);
        auto codecs = reinterpret_cast<Gdiplus::ImageCodecInfo*>(storage.data());
        if (Gdiplus::GetImageEncoders(count, size, codecs) != Gdiplus::Ok)
        {
            return false;
        }
        CLSID encoder = {};
        bool found = false;
        for (UINT index = 0; index < count; ++index)
        {
            if (wcscmp(codecs[index].MimeType, L"image/png") == 0)
            {
                encoder = codecs[index].Clsid;
                found = true;
                break;
            }
        }
        if (!found)
        {
            return false;
        }

        GUID identifier = {};
        wchar_t suffix[40] = {};
        if (FAILED(CoCreateGuid(&identifier)) || !StringFromGUID2(identifier, suffix, 40))
        {
            return false;
        }
        const std::wstring temporary = path + L"." + suffix + L".tmp";
        HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
        {
            return false;
        }
        CloseHandle(file);
        Gdiplus::Bitmap image(bitmap, nullptr);
        bool saved = image.GetLastStatus() == Gdiplus::Ok && image.Save(temporary.c_str(), &encoder, nullptr) == Gdiplus::Ok;
        if (saved)
        {
            saved = MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
        }
        if (!saved && !DeleteFileW(temporary.c_str()))
        {
            OutputDebugStringW(L"PowerToolbar: screenshot temporary file cleanup failed.\n");
        }
        return saved;
    }

    HRESULT ChooseDestination(std::wstring& path)
    {
        Microsoft::WRL::ComPtr<IFileSaveDialog> dialog;
        HRESULT result = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
        if (FAILED(result))
        {
            return result;
        }
        const COMDLG_FILTERSPEC filters[] = {{L"PNG (*.png)", L"*.png"}};
        result = dialog->SetFileTypes(1, filters);
        if (SUCCEEDED(result))
        {
            result = dialog->SetDefaultExtension(L"png");
        }
        if (SUCCEEDED(result))
        {
            result = dialog->SetTitle(Tr(Text::Screenshot));
        }
        if (SUCCEEDED(result))
        {
            result = dialog->SetOptions(FOS_FORCEFILESYSTEM | FOS_OVERWRITEPROMPT | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR);
        }
        if (FAILED(result))
        {
            return result;
        }

        SYSTEMTIME time = {};
        GetLocalTime(&time);
        wchar_t name[80] = {};
        swprintf_s(name, L"PowerToolbar-%04u%02u%02u-%02u%02u%02u.png", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
        result = dialog->SetFileName(name);
        if (FAILED(result))
        {
            return result;
        }
        result = dialog->Show(g_toolbar);
        if (FAILED(result))
        {
            return result;
        }
        Microsoft::WRL::ComPtr<IShellItem> item;
        result = dialog->GetResult(&item);
        if (FAILED(result))
        {
            return result;
        }
        PWSTR filename = nullptr;
        result = item->GetDisplayName(SIGDN_FILESYSPATH, &filename);
        if (SUCCEEDED(result))
        {
            path = filename;
            CoTaskMemFree(filename);
        }
        return result;
    }
}

void SaveWindowScreenshot(HWND hwnd)
{
    if (!IsManageableWindow(hwnd))
    {
        return;
    }
    const bool previousMenuState = g_menuOpen;
    g_menuOpen = true;
    // Remove the on-demand fullscreen controls before capturing client pixels.
    HideToolbarForCapture();
    HBITMAP bitmap = nullptr;
    if (SUCCEEDED(DwmFlush()))
    {
        bitmap = CaptureContent(hwnd);
    }
    if (!bitmap)
    {
        OutputDebugStringW(L"PowerToolbar: window content capture failed.\n");
        MessageBoxW(g_toolbar, Tr(Text::ScreenshotFailed), L"PowerToolbar", MB_OK | MB_ICONWARNING);
        g_menuOpen = previousMenuState;
        UpdateToolbarPosition();
        return;
    }
    std::wstring path;
    const HRESULT result = ChooseDestination(path);
    if (result != HRESULT_FROM_WIN32(ERROR_CANCELLED))
    {
        if (FAILED(result) || !WritePng(bitmap, path))
        {
            OutputDebugStringW(L"PowerToolbar: screenshot save failed.\n");
            MessageBoxW(g_toolbar, Tr(Text::ScreenshotFailed), L"PowerToolbar", MB_OK | MB_ICONWARNING);
        }
    }
    DeleteObject(bitmap);
    g_menuOpen = previousMenuState;
    UpdateToolbarPosition();
}
