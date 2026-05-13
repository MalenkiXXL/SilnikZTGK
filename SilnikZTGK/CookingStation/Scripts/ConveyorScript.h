#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include <glm/glm.hpp>
#include <CookingStation/Core/Input.h>

struct AngleDirection {
    float angle;
    glm::vec3 direction;
};

// Poprawione mapowanie - upewnij siê, ¿e osie pasuj¹ do Twojego œwiata
static constexpr AngleDirection s_Mappings[] = {
    {  90.0f, { 0.0f, 0.0f,  1.0f } },
    { 270.0f, { 0.0f, 0.0f, -1.0f } },
    { 180.0f, { 1.0f, 0.0f,  0.0f } }, // Poprawiono z {1,0,1} na {1,0,0}
    {   0.0f, {-1.0f, 0.0f,  0.0f } },
};

class ConveyorScript : public ScriptableEntity
{
public:
    glm::vec3 PushDirection = { 0.0f, 0.0f, 0.0f };
    float Speed = 2.0f;

    bool IsOccupied = false;
    bool IsJammed = false;

    void OnCreate() override
    {
        SetPushDirection();
    }

    void SetPushDirection()
    {
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        float rotY = transform->GetRotation().y;

        float normalizedRot = fmodf(rotY, 360.0f);
        if (normalizedRot < 0.0f) normalizedRot += 360.0f;

        for (auto& m : s_Mappings)
        {
            if (std::abs(normalizedRot - m.angle) < 5.0f)
            {
                PushDirection = m.direction;
                return;
            }
        }

        PushDirection = s_Mappings[3].direction;
    }

    void OnClick() override
    {
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        auto& conveyorMap = GetScene()->GetConveyorMap();

        // Logika szukania s¹siadów do zwrotnicy
        float validAngles[4];
        int validCount = 0;
        int neighborCount = 0;

        glm::vec3 myPos = transform->GetPosition();

        for (auto& m : s_Mappings)
        {
            // Klucz mapy to pozycja w kratkach (zak³adaj¹c CELL_SIZE = 2.0)
            GridPos neighborKey{
                (int)std::round((myPos.x + m.direction.x * 2.0f) / 2.0f),
                (int)std::round((myPos.z + m.direction.z * 2.0f) / 2.0f)
            };

            auto it = conveyorMap.find(neighborKey);
            if (it == conveyorMap.end()) continue;

            neighborCount++;

            // Nie skrêcajmy "pod pr¹d" s¹siada (czo³ówka)
            ConveyorScript* neighbor = it->second;
            bool isHeadOn = (glm::dot(m.direction, neighbor->PushDirection) < -0.9f);

            if (!isHeadOn)
            {
                validAngles[validCount++] = m.angle;
            }
        }

        // Zmiana kierunku tylko jeœli to faktycznie skrzy¿owanie/zwrotnica
        if (neighborCount >= 3 && validCount > 0)
        {
            float currentRot = transform->GetRotation().y;
            currentRot = fmodf(currentRot, 360.0f);
            if (currentRot < 0.0f) currentRot += 360.0f;

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

            glm::vec3 newRot = transform->GetRotation();
            newRot.y = validAngles[nextIndex];
            transform->SetRotation(newRot);

            SetPushDirection();
            spdlog::info("Zwrotnica: Nowy kierunek: {}", newRot.y);
        }
    }
};