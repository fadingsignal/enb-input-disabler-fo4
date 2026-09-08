#include "pch.h"
#include "ENB.h"
#include "Runtime.h"

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* f4se)
{
    if (!f4se || f4se->IsEditor() || !ENBInputDisabler::IsSupported(f4se->RuntimeVersion())) return false;
    F4SE::Init(f4se, {.log = true, .logName = "ENBInputDisablerFO4", .hook = false});
    REX::INFO("ENB Input Disabler FO4 {}; runtime {}", F4SE::GetPluginVersion().string(),
              f4se->RuntimeVersion().string());
    ENBInputDisabler::ConnectENB();
    // Retry after startup if ENB was not ready during plugin loading.
    if (const auto* messaging = F4SE::GetMessagingInterface()) {
        const bool registered = messaging->RegisterListener([](F4SE::MessagingInterface::Message* message) {
            if (message->type == F4SE::MessagingInterface::kGameLoaded) {
                ENBInputDisabler::ConnectENB();
            }
        });
        if (!registered) REX::WARN("Could not register ENB startup retry listener");
    } else {
        REX::WARN("F4SE messaging unavailable; ENB startup retry disabled");
    }
    return true;
}

extern "C" F4SE_EXPORT bool F4SEPlugin_Query(const F4SE::QueryInterface* f4se, F4SE::PluginInfo* info)
{
    if (!f4se || !info) return false;
    info->infoVersion = F4SE::PluginInfo::kVersion;
    info->name = "ENBInputDisablerFO4";
    const auto* version = F4SE::PluginVersionData::GetSingleton();
    if (!version) return false;
    info->version = version->GetPluginVersion().pack();
    return !f4se->IsEditor() && ENBInputDisabler::IsSupported(f4se->RuntimeVersion());
}
