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

    void OnCreate() override
    {
        SetPushDirection();

        auto* existingScroll = GetComponent<UVScrollComponent>();

        if (!existingScroll)
        {
            UVScrollComponent scroll;
            scroll.Speed = Speed;
            scroll.Offset = 0.0f;
            AddComponent<UVScrollComponent>(scroll);
        }
    }

    void OnUpdate(Timestep ts) override
    {
        auto* scroll = GetComponent<UVScrollComponent>();
        if (scroll)
        {
            // Przesuwamy offset tekstury
            // modelLength 4.0 oznacza, ¿e tekstura powtarza siê co 4 jednostki œwiata
            float modelLength = 4.0f;
            float uvSpeed = Speed / modelLength;

            scroll->Offset += uvSpeed * ts;

            // Zawijanie offsetu (0.0 - 1.0)
            if (scroll->Offset > 1.0f) scroll->Offset -= 1.0f;
            if (scroll->Offset < 0.0f) scroll->Offset += 1.0f;
        }
    }

    void SetPushDirection()
    {
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        // Bierzemy SWOJ¥ rotacjê (któr¹ za chwilê wygeneruje nam Python)
        float rotY = transform->GetRotation().y;

        float normalizedRot = fmodf(rotY, 360.0f);
        if (normalizedRot < 0.0f) normalizedRot += 360.0f;

        for (auto& m : s_Mappings)
        {
            // Zostawiamy tolerancjê 5 stopni, ¿eby uodporniæ siê na b³êdy zmiennoprzecinkowe
            if (std::abs(normalizedRot - m.angle) < 5.0f)
            {
                PushDirection = m.direction;
                return;
            }
        }

        // Domyœlny kierunek
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