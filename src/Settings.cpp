#include <fstream>
#include <string>

#include "Common.h"

namespace REQ::MassEffectArmor {
    namespace {
        SettingsData g_settings{};

        std::string Trim(std::string a_value) {
            auto first = a_value.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) {
                return {};
            }
            auto last = a_value.find_last_not_of(" \t\r\n");
            return a_value.substr(first, last - first + 1);
        }

        RE::FormID ParseRuntimeFormID(const std::string& a_value) {
            try {
                return static_cast<RE::FormID>(std::stoul(a_value, nullptr, 0));
            } catch (...) {
                return 0;
            }
        }

        RE::FormID ParseFormSpec(const std::string& a_value) {
            auto value = Trim(a_value);
            if (value.empty()) {
                return 0;
            }

            const auto sep = value.find('|');
            if (sep == std::string::npos) {
                return ParseRuntimeFormID(value);
            }

            auto modName = Trim(value.substr(0, sep));
            auto localIDText = Trim(value.substr(sep + 1));
            if (modName.empty() || localIDText.empty()) {
                return 0;
            }

            RE::FormID localID = 0;
            try {
                localID = static_cast<RE::FormID>(std::stoul(localIDText, nullptr, 0));
            } catch (...) {
                return 0;
            }

            auto* data = RE::TESDataHandler::GetSingleton();
            if (!data) {
                return 0;
            }

            if (auto* form = data->LookupForm<RE::TESForm>(localID, modName)) {
                return form->GetFormID();
            }
            return 0;
        }
    }

    const SettingsData& GetSettings() { return g_settings; }

    void LoadSettings() {
        g_settings = {};
        std::ifstream in("Data/SKSE/Plugins/REQ_MassEffectArmor-3BFTweaks.ini");
        if (!in.good()) {
            return;
        }

        std::string line;
        while (std::getline(in, line)) {
            auto pos = line.find('=');
            if (pos == std::string::npos) {
                continue;
            }
            auto key = Trim(line.substr(0, pos));
            auto value = Trim(line.substr(pos + 1));
            if (key == "ShieldEmassPerk") {
                g_settings.shieldEmassPerk = ParseFormSpec(value);
            } else if (key == "LordStoneEffect") {
                g_settings.lordStoneEffect = ParseFormSpec(value);
            }
        }
    }
}
