#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include <spdlog/spdlog.h>

class RotationScript : public ScriptableEntity
{
private:
    bool m_IsSpinning = false;

public:

    void OnCollision() override
    {
        m_IsSpinning = true;
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