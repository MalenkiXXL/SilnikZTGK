#include "CookingStation/Scripts/Delivery/PackageScript.h"
#include "CookingStation/Scripts/ParticleEmitterScript.h"
#include "CookingStation/Events/GameEvents.h"
#include <spdlog/spdlog.h>
#include "CookingStation/Core/AudioEngine.h"


void PackageScript::StartPoof() {
    m_IsPoofing = true;
    m_PoofTimer = 2.0f;

    auto *nsc = GetComponent<NativeScriptComponent>();
    if (nsc) {
        for (auto &s: nsc->Scripts) {
            if (s.Name == "PoofEmitterScript" && s.Instance) {
                auto *emitter = static_cast<ParticleEmitterScript *>(s.Instance);
                emitter->Play();
                break;
            }
        }
    }
}

void PackageScript::HandleClick() {
    if (m_IsCollected) return;
    m_IsCollected = true;

    GetScene()->GetWorld().GetEventBus().Publish(AddIngredientEvent{m_Type, m_IngredientAmount});
    GetScene()->GetWorld().GetEventBus().Publish(DeliveryCollectedEvent{});
    AudioEngine::Play("assets://sounds/handleSmallLeather.mp3");
    spdlog::info("Gracz zebrał paczkę");

    std::vector<Entity> allPackages = s_ActivePackages;
    s_ActivePackages.clear();

    for (Entity e: allPackages) {
        if (e.id == m_Entity.id) {
            GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{e});
        } else {
            auto *nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(e);
            if (nsc) {
                for (auto &script: nsc->Scripts) {
                    if (script.Name == "PackageScript" && script.Instance) {
                        ((PackageScript *) script.Instance)->StartPoof();
                        break;
                    }
                }
            }
        }
    }
}

IngredientType PackageScript::getType() const {
    return m_Type;
}
