#ifndef SILNIKZTGK_PACKAGESCRIPT_H
#define SILNIKZTGK_PACKAGESCRIPT_H
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scripts/Managers/IngredientType.h"
#include "CookingStation/Events/GameEvents.h"

class PackageScript : public ScriptableEntity{
public:
    inline static std::vector<Entity> s_ActivePackages;

    IngredientType m_Type = IngredientType::Tomato;

    int m_IngredientAmount = 5;
    void OnUpdate(Timestep ts) override {};

    void HandleClick();

    void OnCreate() override {
        s_ActivePackages.push_back(m_Entity);

        m_ClickSubId = GetScene()->GetWorld().GetEventBus().Subscribe<EntityClickedEvent>(
            [this](const EntityClickedEvent& e) {
                if (e.TargetEntity.id == m_Entity.id) {
                    this->HandleClick();
                }
            }
        );
    }

    void OnDestroy() override {
        GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityClickedEvent>(m_ClickSubId);
    }

private:
    std::size_t m_ClickSubId = 0;
};


#endif //SILNIKZTGK_PACKAGESCRIPT_H
