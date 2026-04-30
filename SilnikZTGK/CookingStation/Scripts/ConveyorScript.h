#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include <glm/glm.hpp>
#include <CookingStation/Core/Input.h>


class ConveyorScript : public ScriptableEntity
{
public:
    glm::vec3 PushDirection = { 0.0f, 0.0f, 0.0f };
    float Speed = 2.0f;
    bool IsSwich = true;

    void OnCreate() override
    {
        SetPushDirection();
    }

    void SetPushDirection()
    {
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        float rotY = transform->Rotation.y;

        while (rotY < 0.0f) rotY += 360.0f;
        while (rotY >= 360.0f) rotY -= 360.0f;

        if (std::abs(rotY - 90.0f) < 1.0f) PushDirection = { 1.0f, 0.0f,  0.0f };
        else if (std::abs(rotY - 270.0f) < 1.0f) PushDirection = { -1.0f, 0.0f,  0.0f };
        else if (std::abs(rotY - 180.0f) < 1.0f) PushDirection = { 0.0f, 0.0f, -1.0f };
        else                                      PushDirection = { 0.0f, 0.0f,  1.0f };
    }

    void OnUpdate(Timestep ts) override
    {

    }


    void OnClick() override
    {
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        auto& conveyorMap = GetScene()->GetConveyorMap(); // potrzebujesz gettera w Scene.h

        // --- 1. SKANER S¥SIADÓW - zamiast pêtli po wszystkich skryptach ---
        int neighborCount = 0;

        float directions[4][2] = { {0,1}, {1,0}, {0,-1}, {-1,0} }; // N, E, S, W

        for (auto& dir : directions)
        {
            GridPos neighborKey{
                (int)std::round((transform->Position.x + dir[0] * 2.0f) / 2.0f),
                (int)std::round((transform->Position.z + dir[1] * 2.0f) / 2.0f)
            };

            if (conveyorMap.count(neighborKey) > 0)
                neighborCount++;
        }

        if (neighborCount < 3)
        {
            spdlog::info("Taœma ma tylko {} sasiadow. To nie jest zwrotnica!", neighborCount);
            return;
        }

        // --- 2. ZBIERAMY BEZPIECZNE WYJŒCIA - zamiast drugiej pêtli ---
        float validAngles[4];
        int validCount = 0;

        float testAngles[4] = { 0.0f, 90.0f, 180.0f, 270.0f };
        glm::vec3 testDirections[4] = {
            {0,0,1}, {1,0,0}, {0,0,-1}, {-1,0,0}
        };

        for (int i = 0; i < 4; i++)
        {
            glm::vec3 testDir = testDirections[i];
            GridPos neighborKey{
                (int)std::round((transform->Position.x + testDir.x * 2.0f) / 2.0f),
                (int)std::round((transform->Position.z + testDir.z * 2.0f) / 2.0f)
            };

            auto it = conveyorMap.find(neighborKey);
            if (it == conveyorMap.end()) continue; // brak taœmy w tym kierunku

            ConveyorScript* neighbor = it->second;
            // Anty-kolizja - bez dynamic_cast!
            bool isHeadOnCollision = (
                std::abs(neighbor->PushDirection.x + testDir.x) < 0.1f &&
                std::abs(neighbor->PushDirection.z + testDir.z) < 0.1f
                );

            if (!isHeadOnCollision)
            {
                validAngles[validCount] = testAngles[i];
                validCount++;
            }
        }

        // --- 3. PRZE£¥CZ NA NASTÊPNE WYJŒCIE - bez zmian ---
        if (validCount > 0)
        {
            float currentRot = transform->Rotation.y;
            while (currentRot < 0.0f) currentRot += 360.0f;
            while (currentRot >= 360.0f) currentRot -= 360.0f;

            int currentIndex = -1;
            for (int i = 0; i < validCount; i++)
            {
                if (std::abs(validAngles[i] - currentRot) < 1.0f)
                {
                    currentIndex = i;
                    break;
                }
            }

            int nextIndex = (currentIndex + 1) % validCount;
            transform->Rotation.y = validAngles[nextIndex];
            SetPushDirection();
        }
    }
};