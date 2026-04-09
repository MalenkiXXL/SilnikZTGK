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
                transform->Rotation.y += 90.0f * ts;
            }
        }
    }

};