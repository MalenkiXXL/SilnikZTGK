#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scripts/ConveyorScript.h"
#include <cmath>

class ItemScript : public ScriptableEntity
{
    ConveyorScript* m_CurrentConveyor = nullptr;

public:
    void OnCreate() override {}

    void OnUpdate(Timestep ts) override
    {
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        ConveyorScript* found = GetScene()->GetConveyorAt(
            transform->Position.x,
            transform->Position.z
        );

        if (found && found != m_CurrentConveyor)
        {
            auto* foundTransform = found->GetComponent<TransformComponent>();
            if (foundTransform)
            {
                float dx = transform->Position.x - foundTransform->Position.x;
                float dz = transform->Position.z - foundTransform->Position.z;

                if (dx * dx + dz * dz < 0.25f) // 0.5f * 0.5f
                    m_CurrentConveyor = found;
            }
        }
        else if (found)
        {
            m_CurrentConveyor = found;
        }

        if (!m_CurrentConveyor) return;

        auto* conveyorTransform = m_CurrentConveyor->GetComponent<TransformComponent>();
        float speed = m_CurrentConveyor->Speed * ts.GetSeconds();
        float diffX = conveyorTransform->Position.x - transform->Position.x;
        float diffZ = conveyorTransform->Position.z - transform->Position.z;

        if (m_CurrentConveyor)
        {
            auto* conveyorTransform = m_CurrentConveyor->GetComponent<TransformComponent>();
            float speed = m_CurrentConveyor->Speed * ts.GetSeconds();

            float diffX = conveyorTransform->Position.x - transform->Position.x;
            float diffZ = conveyorTransform->Position.z - transform->Position.z;

            if (std::abs(m_CurrentConveyor->PushDirection.x) > 0.1f)
            {
                if (std::abs(diffZ) > 0.01f) {
                    if (std::abs(diffZ) <= speed) transform->Position.z = conveyorTransform->Position.z;
                    else transform->Position.z += std::copysign(speed, diffZ);
                }
                else {
                    transform->Position.x += m_CurrentConveyor->PushDirection.x * speed;
                    transform->Position.z = conveyorTransform->Position.z;
                }
            }
            else if (std::abs(m_CurrentConveyor->PushDirection.z) > 0.1f)
            {
                if (std::abs(diffX) > 0.01f) {
                    if (std::abs(diffX) <= speed) transform->Position.x = conveyorTransform->Position.x;
                    else transform->Position.x += std::copysign(speed, diffX);
                }
                else {
                    transform->Position.z += m_CurrentConveyor->PushDirection.z * speed;
                    transform->Position.x = conveyorTransform->Position.x;
                }
            }
        }
    }
};