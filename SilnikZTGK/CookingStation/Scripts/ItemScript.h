#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scripts/ConveyorScript.h"
#include <cmath>

class ItemScript : public ScriptableEntity
{
public:
    void OnCreate() override {}

    void OnUpdate(Timestep ts) override
    {
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        auto& conveyors = GetScene()->GetConveyors();

        ConveyorScript* closestConveyor = nullptr;
        float minDistance = 1.0f;

        for (ConveyorScript* conveyor : conveyors)
        {
            auto* conveyorTransform = conveyor->GetComponent<TransformComponent>();
            if (!conveyorTransform) continue;

            float distX = transform->Position.x - conveyorTransform->Position.x;
            float distZ = transform->Position.z - conveyorTransform->Position.z;
            float dist = std::sqrt(distX * distX + distZ * distZ);

            if (dist < minDistance)
            {
                minDistance = dist;
                closestConveyor = conveyor;
            }
        }

        if (closestConveyor)
        {
            auto* conveyorTransform = closestConveyor->GetComponent<TransformComponent>();
            float speed = closestConveyor->Speed * ts.GetSeconds();

            float diffX = conveyorTransform->Position.x - transform->Position.x;
            float diffZ = conveyorTransform->Position.z - transform->Position.z;

            if (std::abs(closestConveyor->PushDirection.x) > 0.1f)
            {
                if (std::abs(diffZ) > 0.01f) {
                    if (std::abs(diffZ) <= speed) transform->Position.z = conveyorTransform->Position.z;
                    else transform->Position.z += std::copysign(speed, diffZ);
                }
                else {
                    transform->Position.x += closestConveyor->PushDirection.x * speed;
                    transform->Position.z = conveyorTransform->Position.z;
                }
            }
            else if (std::abs(closestConveyor->PushDirection.z) > 0.1f)
            {
                if (std::abs(diffX) > 0.01f) {
                    if (std::abs(diffX) <= speed) transform->Position.x = conveyorTransform->Position.x;
                    else transform->Position.x += std::copysign(speed, diffX);
                }
                else {
                    transform->Position.z += closestConveyor->PushDirection.z * speed;
                    transform->Position.x = conveyorTransform->Position.x;
                }
            }
        }
    }
};