//
// Created by Amelia on 17.05.2026.
//
#include "SilnikZTGK/CookingStation/Scripts/Delivery/DeliveryCarScript.h"
#include "CookingStation/Scene/PrefabSerializer.h"
#include <spdlog/spdlog.h>

void DeliveryCarScript::OnCreate()
{
    spdlog::info("Dostawczak wyruszył w drogę!");
}

void DeliveryCarScript::OnUpdate(Timestep ts)
{
    auto* transform = GetComponent<TransformComponent>();
    if (!transform) return;

    glm::vec3 currentPos = transform->GetPosition();

    switch (m_State)
    {
        case DeliveryState::DRIVING_IN:
        {
            glm::vec3 dir = m_DropPos - currentPos;
            float dist = glm::length(dir);

            if (dist < 0.1f)
            {
                transform->SetPosition(m_DropPos);
                m_State = DeliveryState::DROPPING;
                m_WaitTimer = 0.0f;
            }
            else
            {
                dir = glm::normalize(dir);
                currentPos += dir * m_Speed * (float)ts.GetSeconds();
                transform->SetPosition(currentPos);
            }
            break;
        }

        case DeliveryState::DROPPING:
        {
            m_WaitTimer += (float)ts.GetSeconds();
            if (m_WaitTimer >= 1.0f)
            {
                SpawnPackages();
                m_State = DeliveryState::DRIVING_OUT;
            }
            break;
        }

        case DeliveryState::DRIVING_OUT:
        {
            glm::vec3 dir = m_ExitPos - currentPos;
            float dist = glm::length(dir);

            if (dist < 0.1f)
            {
                spdlog::info("Dostawczak opuścił mapę. Niszczenie encji...");
                GetScene()->GetWorld().DestroyEntity(m_Entity);
            }
            else
            {
                dir = glm::normalize(dir);
                currentPos += dir * m_Speed * (float)ts.GetSeconds();
                transform->SetPosition(currentPos);
            }
            break;
        }
    }
}

void DeliveryCarScript::SpawnPackages()
{
    // 1. Pobierz transformację i obecną pozycję dostawczaka
    auto* transform = GetComponent<TransformComponent>();
    if (!transform) return;

    glm::vec3 currentPos = transform->GetPosition();

    // 2. Przesunięcie względem pozycji auta
    glm::vec3 package1Pos = currentPos + glm::vec3(2.0f, -1.0f, -1.0f);
    glm::vec3 package2Pos = currentPos + glm::vec3(2.0f, -1.0f, 1.0f);

    // 3. Zespawnuj paczki
    PrefabSerializer::Deserialize(GetScene(), m_PackagePrefabPath, package1Pos);
    PrefabSerializer::Deserialize(GetScene(), m_PackagePrefabPath, package2Pos);

    spdlog::info("Wyrzucono 2 paczki! Pozycje: ({}, {}, {}) oraz ({}, {}, {})",
                 package1Pos.x, package1Pos.y, package1Pos.z,
                 package2Pos.x, package2Pos.y, package2Pos.z);
}