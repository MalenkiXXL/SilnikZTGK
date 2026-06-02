#ifndef SILNIKZTGK_PACKAGESCRIPT_H
#define SILNIKZTGK_PACKAGESCRIPT_H
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Events/GameEvents.h"

class PackageScript : public ScriptableEntity{
public:
    inline static std::vector<Entity> s_ActivePackages;

    IngredientType m_Type = IngredientType::Tomato;
    int m_IngredientAmount = 5;
    void OnUpdate(Timestep ts) override {};

    void HandleClick();

    void OnCreate() override {
        spdlog::info("[PackageScript] Utworzono paczke typu {}", (int)m_Type);

        s_ActivePackages.push_back(m_Entity);

        m_ClickSubId = GetScene()->GetWorld().GetEventBus().Subscribe<EntityClickedEvent>(
            [this](const EntityClickedEvent& e) {
                if (e.TargetEntity.id == m_Entity.id) {
                    this->HandleClick();
                }
            }
        );

        m_ConfigSubId = GetScene()->GetWorld().GetEventBus().Subscribe<ConfigurePackageEvent>(
                [this](const ConfigurePackageEvent& e) {
                    if (e.TargetEntity.id == this->m_Entity.id && !this->m_IsConfigured) {
                        this->m_Type = e.Type;
                        this->m_IngredientAmount = e.Amount;
                        this->m_IsConfigured = true;
                        spdlog::info("[PackageScript] Złapałem event! Jestem typem: {}", (int)m_Type);
                    }
                }
        );

        GetScene()->GetWorld().GetEventBus().Publish(PackageSpawnedEvent{ m_Entity });
    }

    void OnDestroy() override {
        GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityClickedEvent>(m_ClickSubId);
        GetScene()->GetWorld().GetEventBus().Unsubscribe<ConfigurePackageEvent>(m_ConfigSubId);
        auto it = std::find_if(s_ActivePackages.begin(), s_ActivePackages.end(),
                               [this](const Entity& e) { return e.id == this->m_Entity.id; });

        if (it != s_ActivePackages.end()) {
            s_ActivePackages.erase(it);
        }
    }

private:
    std::size_t m_ClickSubId = 0;
    bool m_IsCollected = false;

    std::size_t m_ConfigSubId = 0;
    bool m_IsConfigured = false;
};


#endif //SILNIKZTGK_PACKAGESCRIPT_H
