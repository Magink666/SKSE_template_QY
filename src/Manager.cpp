#include <algorithm>
#include <array>
#include <unordered_map>

#include "Common.h"

namespace REQ::MassEffectArmor {
    namespace {
        struct EffectState {
            RE::FormID target{0};
            bool update{false};
            bool busy{false};
            bool finish{false};
            float activeMass{0.0F};
            float activePenalty{0.0F};
        };

        std::unordered_map<RE::FormID, EffectState> g_states;

        float Clamp(float a_value, float a_min, float a_max) { return std::clamp(a_value, a_min, a_max); }

        RE::Actor* ResolveActor(RE::FormID a_formID) {
            return a_formID ? RE::TESForm::LookupByID<RE::Actor>(a_formID) : nullptr;
        }

        std::array<RE::TESObjectARMO*, 5> GetEquippedArmors(RE::Actor* a_actor) {
            std::array<RE::TESObjectARMO*, 5> armors{};
            armors[0] = a_actor->GetWornArmor(RE::BIPED_MODEL::BipedObjectSlot::kBody);
            armors[1] = a_actor->GetWornArmor(RE::BIPED_MODEL::BipedObjectSlot::kHair);
            armors[2] = a_actor->GetWornArmor(RE::BIPED_MODEL::BipedObjectSlot::kHands);
            armors[3] = a_actor->GetWornArmor(RE::BIPED_MODEL::BipedObjectSlot::kFeet);
            armors[4] = a_actor->GetWornArmor(RE::BIPED_MODEL::BipedObjectSlot::kShield);
            for (std::size_t i = 0; i < armors.size(); ++i) {
                if (!armors[i]) {
                    continue;
                }
                for (std::size_t j = i + 1; j < armors.size(); ++j) {
                    if (armors[i] == armors[j]) {
                        armors[j] = nullptr;
                    }
                }
            }
            return armors;
        }

        std::array<float, 3> GetWeightClassFactors(RE::Actor* a_actor) {
            auto* avo = a_actor->AsActorValueOwner();
            if (!avo) {
                return {1.0F, 1.0F, 1.0F};
            }
            const auto heavy = Clamp(avo->GetActorValue(RE::ActorValue::kHeavyArmorModifier), 0.0F, 100.0F);
            const auto light = Clamp(avo->GetActorValue(RE::ActorValue::kLightArmorModifier), 0.0F, 100.0F);
            return {1.0F - light * 0.01F, 1.0F - heavy * 0.01F, 1.0F - light * 0.01F};
        }

        void UpdateMass(EffectState& a_state, float a_newMass) {
            auto* actor = ResolveActor(a_state.target);
            auto* avo = actor ? actor->AsActorValueOwner() : nullptr;
            if (!avo || a_newMass == a_state.activeMass) {
                return;
            }
            const auto diffMass = a_newMass - a_state.activeMass;
            avo->ModActorValue(RE::ActorValue::kMass, diffMass);
            avo->ModActorValue(RE::ActorValue::kAlterationSkillAdvance, 100.0F * diffMass);
            a_state.activeMass = a_newMass;
        }

        void UpdatePenalty(EffectState& a_state, float a_newPenalty) {
            auto* actor = ResolveActor(a_state.target);
            auto* avo = actor ? actor->AsActorValueOwner() : nullptr;
            if (!avo || a_newPenalty == a_state.activePenalty) {
                return;
            }
            const auto diffPenalty = a_newPenalty - a_state.activePenalty;
            avo->ModActorValue(RE::ActorValue::kInfamy, diffPenalty);
            avo->ModActorValue(RE::ActorValue::kMovementNoiseMult, diffPenalty);
            avo->ModActorValue(RE::ActorValue::kSpeedMult, -50.0F * diffPenalty);
            a_state.activePenalty = a_newPenalty;
        }

        void UpdateMassEffect(EffectState& a_state) {
            a_state.busy = true;
            a_state.update = false;

            auto* actor = ResolveActor(a_state.target);
            if (!actor) {
                a_state.busy = false;
                return;
            }

            if (a_state.finish) {
                UpdateMass(a_state, 0.0F);
                UpdatePenalty(a_state, 0.0F);
                a_state.busy = false;
                return;
            }

            const auto armors = GetEquippedArmors(actor);
            const auto factors = GetWeightClassFactors(actor);
            const auto& settings = GetSettings();
            auto* shieldPerk =
                settings.shieldEmassPerk ? RE::TESForm::LookupByID<RE::BGSPerk>(settings.shieldEmassPerk) : nullptr;
            auto* lordStone = settings.lordStoneEffect
                                  ? RE::TESForm::LookupByID<RE::EffectSetting>(settings.lordStoneEffect)
                                  : nullptr;

            float newMass = 0.0F;
            float newPenalty = 0.0F;
            for (std::size_t i = 0; i < armors.size(); ++i) {
                auto* armor = armors[i];
                if (!armor) {
                    continue;
                }
                const auto wc = std::to_underlying(armor->GetArmorType());
                if (wc < 0 || wc >= static_cast<int>(factors.size())) {
                    continue;
                }
                const auto mass = armor->GetWeight() * 0.01F;
                newMass += mass;
                if (i == 4 && shieldPerk && actor->HasPerk(shieldPerk)) {
                    newPenalty += 0.25F * mass;
                } else {
                    newPenalty += factors[wc] * mass;
                }
            }

            auto* mt = actor->AsMagicTarget();
            if (lordStone && mt && mt->HasMagicEffect(lordStone)) {
                newPenalty *= 0.66F;
            }

            UpdateMass(a_state, newMass);
            UpdatePenalty(a_state, newPenalty);

            a_state.busy = false;
            if (a_state.update) {
                UpdateMassEffect(a_state);
            }
        }

        void QueueUpdate(EffectState& a_state) {
            if (a_state.busy) {
                a_state.update = true;
                return;
            }
            UpdateMassEffect(a_state);
        }
    }

    Manager* GetManager() {
        static Manager singleton;
        return std::addressof(singleton);
    }

    void Manager::Start(RE::Actor* a_actor) {
        if (!a_actor) {
            return;
        }
        auto& state = g_states[a_actor->GetFormID()];
        state.target = a_actor->GetFormID();
        state.finish = false;
        QueueUpdate(state);
    }

    void Manager::Stop(RE::Actor* a_actor) {
        if (!a_actor) {
            return;
        }
        auto it = g_states.find(a_actor->GetFormID());
        if (it == g_states.end()) {
            return;
        }
        it->second.finish = true;
        QueueUpdate(it->second);
    }

    void Manager::OnStatsMenuClosed() {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return;
        }
        auto it = g_states.find(player->GetFormID());
        if (it != g_states.end()) {
            QueueUpdate(it->second);
        }
    }

    void Manager::OnObjectChanged(RE::Actor* a_actor, RE::FormID a_object) {
        if (!a_actor || a_object == 0) {
            return;
        }
        auto* form = RE::TESForm::LookupByID(a_object);
        if (!form || !form->As<RE::TESObjectARMO>()) {
            return;
        }
        auto it = g_states.find(a_actor->GetFormID());
        if (it != g_states.end()) {
            QueueUpdate(it->second);
        }
    }
}
