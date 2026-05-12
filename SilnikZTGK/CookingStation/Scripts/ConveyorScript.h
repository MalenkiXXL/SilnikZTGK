#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include <glm/glm.hpp>
#include <CookingStation/Core/Input.h>

struct AngleDirection {
    float angle;
    glm::vec3 direction;
};

static constexpr AngleDirection s_Mappings[] = {
    {  90.0f, { 0.0f, 0.0f,  1.0f } },
    { 270.0f, { 0.0f, 0.0f, -1.0f } },
    { 180.0f, { 1.0f, 0.0f,  1.0f } }, // Poprawione na wektory osiowe jeœli trzeba
    {   0.0f, {-1.0f, 0.0f,  0.0f } },
};

class ConveyorScript : public ScriptableEntity
{
public:
    glm::vec3 PushDirection = { 0.0f, 0.0f, 0.0f };
    float Speed = 2.0f;
    bool IsSwich = true;

    void OnCreate() override
    {
        SetPushDirection();

        // Dodajemy komponent do animacji tekstury
        UVScrollComponent scroll;
        scroll.Speed = Speed;
        AddComponent<UVScrollComponent>(scroll);
    }

    void OnUpdate(Timestep ts) override
    {
        auto* scroll = GetComponent<UVScrollComponent>();
        if (scroll)
        {
            // Logika animacji UV (przesuwanie taœmy)
            float modelLength = 4.0f;
            float uvSpeed = Speed / modelLength;

            scroll->Offset += uvSpeed * ts;
            scroll->Offset = fmodf(scroll->Offset, 1.0f);

            if (scroll->Offset < 0.0f)
                scroll->Offset += 1.0f;
        }
    }

    void SetPushDirection()
    {
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        float rotY = transform->GetRotation().y;

        while (rotY < 0.0f) rotY += 360.0f;
        while (rotY >= 360.0f) rotY -= 360.0f;

        for (auto& m : s_Mappings)
            if (std::abs(rotY - m.angle) < 1.0f)
            {
                PushDirection = m.direction;
                return;
            }
        PushDirection = s_Mappings[3].direction;
    }

    void OnClick() override
    {
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        auto& conveyorMap = GetScene()->GetConveyorMap();
        int neighborCount = 0;

        float validAngles[4];
        int validCount = 0;

        glm::vec3 myPos = transform->GetPosition();

        for (auto& m : s_Mappings)
        {
            GridPos neighborKey{
                (int)std::round((myPos.x + m.direction.x * 2.0f) / 2.0f),
                (int)std::round((myPos.z + m.direction.z * 2.0f) / 2.0f)
            };

            auto it = conveyorMap.find(neighborKey);
            if (it == conveyorMap.end()) continue;

            neighborCount++;

            ConveyorScript* neighbor = it->second;
            bool isHeadOnCollision = (
                std::abs(neighbor->PushDirection.x + m.direction.x) < 0.1f &&
                std::abs(neighbor->PushDirection.z + m.direction.z) < 0.1f
                );
            if (!isHeadOnCollision)
            {
                validAngles[validCount] = m.angle;
                validCount++;
            }
        }

        if (neighborCount < 3)
        {
            spdlog::info("Tasma ma tylko {} sasiadow. To nie jest zwrotnica!", neighborCount);
            return;
        }

        if (validCount > 0)
        {
            glm::vec3 currentRotVec = transform->GetRotation();
            float currentRot = currentRotVec.y;

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

            currentRotVec.y = validAngles[nextIndex];
            transform->SetRotation(currentRotVec);

            SetPushDirection();
        }
    }
};