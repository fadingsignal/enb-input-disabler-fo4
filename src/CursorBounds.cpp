#include "pch.h"
#include "CursorBounds.h"
#include <atomic>
#include <filesystem>
#include <REX/FModule.h>

namespace ENBInputDisabler
{
namespace
{
using ClipCursorFunction = BOOL(WINAPI*)(const RECT*);
ClipCursorFunction originalClipCursor{};
std::atomic_bool editorActive{};
std::atomic_bool loggedExpansion{};

BOOL WINAPI ClipGameCursor(const RECT* requested)
{
    if (!requested || !editorActive.load(std::memory_order_acquire))
        return originalClipCursor(requested);

    // Window queries and logging must not change the Win32 error state observed
    // by the caller or by another ClipCursor wrapper already installed.
    const DWORD incomingError = GetLastError();
    const auto forward = [&](const RECT* rect) {
        SetLastError(incomingError);
        return originalClipCursor(rect);
    };

    const HWND window = GetForegroundWindow();
    DWORD process{};
    if (!window || !GetWindowThreadProcessId(window, &process) ||
        process != GetCurrentProcessId() || IsIconic(window))
        return forward(requested);

    RECT client{};
    POINT origin{};
    if (!GetClientRect(window, &client) || !ClientToScreen(window, &origin))
        return forward(requested);
    OffsetRect(&client, origin.x, origin.y);
    if (!ShouldExpandClip({requested->left, requested->top, requested->right, requested->bottom},
                          {client.left, client.top, client.right, client.bottom}))
        return forward(requested);

    const auto result = forward(&client);
    const DWORD resultError = GetLastError();
    if (result && !loggedExpansion.exchange(true, std::memory_order_relaxed)) {
        REX::INFO("ENB cursor bounds expanded: [{},{},{},{}] -> client [{},{},{},{}]",
                  requested->left, requested->top, requested->right, requested->bottom,
                  client.left, client.top, client.right, client.bottom);
    }
    SetLastError(resultError);
    return result;
}
}

void SetCursorEditorActive(bool active)
{
    if (editorActive.exchange(active, std::memory_order_acq_rel) != active)
        loggedExpansion.store(false, std::memory_order_relaxed);
}

bool InstallCursorBoundsFix()
{
    static bool initialized = false;
    if (initialized) return true;

    // Read beside this DLL, including installations managed by a mod manager.
    HMODULE plugin{};
    std::array<wchar_t, 32768> path{};
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&InstallCursorBoundsFix), &plugin)) {
        REX::WARN("Cannot locate plugin module for cursor INI: {}", GetLastError());
        return false;
    }
    const auto length = GetModuleFileNameW(plugin, path.data(), static_cast<DWORD>(path.size()));
    if (!length || length >= path.size()) {
        REX::WARN("Cannot read plugin path for cursor INI: {}", GetLastError());
        return false;
    }
    auto ini = std::filesystem::path(path.data()).replace_extension(L".ini");
    if (!GetPrivateProfileIntW(L"Compatibility", L"FixCursorBounds", 1, ini.c_str())) {
        REX::INFO("Cursor bounds compatibility fix disabled by INI");
        initialized = true;
        return true;
    }

    // Patch only Fallout4.exe's import. ENB and the upscaler keep their own
    // imports. Preserve any existing wrapper as the original function.
    const auto game = REX::FModule::GetExecutingModule();
    auto* slot = static_cast<ClipCursorFunction*>(game.GetImportFunctionPointer("ClipCursor", "USER32.dll"));
    if (!slot || !*slot) {
        REX::WARN("Cursor bounds fix unavailable: Fallout4.exe ClipCursor import missing");
        return false;
    }
    DWORD previousProtect{};
    if (!VirtualProtect(slot, sizeof(*slot), PAGE_READWRITE, &previousProtect)) {
        REX::WARN("Cursor bounds fix unavailable: cannot write import slot");
        return false;
    }
    originalClipCursor = *slot;
    *slot = &ClipGameCursor;
    DWORD unused{};
    if (!VirtualProtect(slot, sizeof(*slot), previousProtect, &unused))
        REX::WARN("Could not restore ClipCursor import page protection: {}", GetLastError());
    initialized = true;
    REX::INFO("Installed ENB cursor bounds compatibility fix");
    return true;
}
}
