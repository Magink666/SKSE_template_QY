#pragma once

#include <RE/Skyrim.h>

namespace REQ::MassEffectArmor {
    struct SettingsData {
        RE::FormID shieldEmassPerk{0};
        RE::FormID lordStoneEffect{0};
    };

    const SettingsData& GetSettings();

    class Manager {
    public:
        void Start(RE::Actor* a_actor);
        void Stop(RE::Actor* a_actor);
        void OnStatsMenuClosed();
        void OnObjectChanged(RE::Actor* a_actor, RE::FormID a_object);
    };

    Manager* GetManager();

    void InstallEvents();
    void InstallHooks();
    void InstallMCP();
    void InstallSerialization();
    void LoadSettings();
    void InitUtils();
}
