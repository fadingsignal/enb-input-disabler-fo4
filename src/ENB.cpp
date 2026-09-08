#include "pch.h"
#include "ENB.h"
#include "InputState.h"
#include "CursorBounds.h"

namespace ENBInputDisabler
{
static_assert(offsetof(RE::ControlMap, ignoreKeyboardMouse) == 0x141);
namespace
{
// ENB SDK 1.x ABI, as used by the original enb-input-disabler.
enum class Callback : long { EndFrame = 1, BeginFrame = 2, OnExit = 6 };
enum class State : long { IsEditorActive = 1 };
using CallbackFunction = void(WINAPI*)(Callback);
using GetSDKVersion = long(*)();
using SetCallbackFunction = void(*)(CallbackFunction);
using GetState = long(*)(State);
GetState getState{};
RE::ControlMap** controlMap{};
InputState inputState;
bool connected{};
bool wasOpen{};

void WINAPI OnENB(Callback type)
{
    if (type != Callback::BeginFrame && type != Callback::OnExit) return;
    // The SDK permits state queries only inside its callback.
    const bool open = type != Callback::OnExit && getState(State::IsEditorActive) != 0;
    SetCursorEditorActive(open);
    auto* map = *controlMap;
    inputState.Update(map ? &map->ignoreKeyboardMouse : nullptr, open);
    if (open != wasOpen) {
        REX::INFO("ENB editor {}", open ? "opened: suppressing game keyboard/mouse input" :
                                       "closed: restoring prior input state");
        wasOpen = open;
    }
}
}

bool ConnectENB()
{
    if (connected) return true;
    std::vector<HMODULE> modules(128);
    DWORD needed{};
    for (;;) {
        const auto bytes = static_cast<DWORD>(modules.size() * sizeof(HMODULE));
        if (!EnumProcessModules(GetCurrentProcess(), modules.data(), bytes, &needed)) {
            REX::WARN("Cannot enumerate ENB modules: {}", GetLastError());
            return false;
        }
        if (needed <= bytes) break;
        modules.resize(needed / sizeof(HMODULE) + 16);
    }
    for (std::size_t i = 0; i < needed / sizeof(HMODULE); ++i) {
        const auto module = modules[i];
        const auto version = reinterpret_cast<GetSDKVersion>(GetProcAddress(module, "ENBGetSDKVersion"));
        if (!version) continue;
        const auto callback = reinterpret_cast<SetCallbackFunction>(GetProcAddress(module, "ENBSetCallbackFunction"));
        const auto state = reinterpret_cast<GetState>(GetProcAddress(module, "ENBGetState"));
        const auto sdk = version();
        if (!callback || !state || sdk < 1001 || sdk >= 2000) {
            REX::WARN("Skipping incompatible ENB SDK {} or missing exports", sdk);
            continue;
        }
        // Resolve before registering: ENB may invoke the callback immediately.
        // VariantID selects OG vs AE; each executable uses its own database.
        REL::Relocation<RE::ControlMap**> singleton{RE::ID::ControlMap::Singleton};
        controlMap = singleton.get();
        if (!InstallCursorBoundsFix()) {
            REX::WARN("Continuing with input suppression; cursor bounds fix could not initialize");
        }
        getState = state;
        connected = true;
        callback(OnENB);
        REX::INFO("Connected to ENB SDK {}; ControlMap singleton at {:X}", sdk, singleton.address());
        return true;
    }
    REX::INFO("ENB SDK not available; game input remains unchanged");
    return false;
}
}
