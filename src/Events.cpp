#include "Common.h"

namespace REQ::MassEffectArmor {
    class EventHandler final : public RE::BSTEventSink<RE::MenuOpenCloseEvent>,
                               public RE::BSTEventSink<RE::TESEquipEvent> {
    public:
        static EventHandler* GetSingleton() {
            static EventHandler singleton;
            return std::addressof(singleton);
        }

        void Install() {
            if (auto* ui = RE::UI::GetSingleton()) {
                ui->AddEventSink<RE::MenuOpenCloseEvent>(this);
            }
            if (auto* source = RE::ScriptEventSourceHolder::GetSingleton()) {
                source->AddEventSink<RE::TESEquipEvent>(this);
            }
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
                                              RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
            if (!a_event || a_event->opening || a_event->menuName != RE::StatsMenu::MENU_NAME) {
                return RE::BSEventNotifyControl::kContinue;
            }
            GetManager()->OnStatsMenuClosed();
            return RE::BSEventNotifyControl::kContinue;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* a_event,
                                              RE::BSTEventSource<RE::TESEquipEvent>*) override {
            if (!a_event || !a_event->actor || a_event->baseObject == 0) {
                return RE::BSEventNotifyControl::kContinue;
            }

            auto* form = RE::TESForm::LookupByID(a_event->baseObject);
            if (!form || !form->As<RE::TESObjectARMO>()) {
                return RE::BSEventNotifyControl::kContinue;
            }

            auto* actor = a_event->actor->As<RE::Actor>();
            if (!actor) {
                return RE::BSEventNotifyControl::kContinue;
            }

            GetManager()->OnObjectChanged(actor, a_event->baseObject);
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    void InstallEvents() { EventHandler::GetSingleton()->Install(); }
}
