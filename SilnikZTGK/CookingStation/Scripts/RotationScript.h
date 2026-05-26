#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include <spdlog/spdlog.h>
#include "CookingStation/Events/GameEvents.h"

class RotationScript : public ScriptableEntity
{
private:
    bool m_IsSpinning = false;
    std::size_t m_CollisionSubId = 0;

public:
    void OnCreate() override
    {
        m_CollisionSubId = GetScene()->GetWorld().GetEventBus().Subscribe<CollisionEvent>(
            [this](const CollisionEvent& e) {
                // Sprawdzamy, czy to my bierzemy udzia³ w kolizji (czy uderzyliœmy my, czy ktoœ w nas)
                if (e.EntityA.id == m_Entity.id || e.EntityB.id == m_Entity.id)
                {
                    this->m_IsSpinning = true;
                }
            }
        );
    }

    void OnDestroy() override
    {
        GetScene()->GetWorld().GetEventBus().Unsubscribe<CollisionEvent>(m_CollisionSubId);
    }
  
    void OnUpdate(Timestep ts) override
    {
        if (m_IsSpinning)
        {
            auto* transform = GetComponent<TransformComponent>();
            if (transform)
            {
                // 1. Pobieramy obecny wektor rotacji
                glm::vec3 currentRot = transform->GetRotation();

                // 2. Modyfikujemy odpowiedni¹ oœ (dodajemy 90 stopni * delta time)
                currentRot.y += 90.0f * ts;

                // 3. Wgrywamy z powrotem przez Setter (To aktywuje flagê IsDirty!)
                transform->SetRotation(currentRot);
            }
        }
    }

};