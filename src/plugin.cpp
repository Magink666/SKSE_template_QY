#include <SKSE/SKSE.h>

#include "Common.h"
#include "logger.h"

namespace REQ::MassEffectArmor {
    void OnMessage(SKSE::MessagingInterface::Message* a_message) {
        if (!a_message) {
            return;
        }

        if (a_message->type == SKSE::MessagingInterface::kDataLoaded) {
            InstallHooks();
            InstallEvents();
            InstallMCP();
            InstallSerialization();
        }

        if (a_message->type == SKSE::MessagingInterface::kNewGame ||
            a_message->type == SKSE::MessagingInterface::kPostLoadGame) {
            LoadSettings();
            InitUtils();
            if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                GetManager()->Start(player);
            }
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse) {
    SetupLog();
    logger::info("MassEffectArmor loaded");

    SKSE::Init(a_skse);
    SKSE::GetMessagingInterface()->RegisterListener(REQ::MassEffectArmor::OnMessage);
    return true;
}
