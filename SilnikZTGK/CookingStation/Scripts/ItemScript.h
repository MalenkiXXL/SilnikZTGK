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

        // 1. Pobieramy obecn¹ pozycjê obiektu
        glm::vec3 myPos = transform->GetPosition();

        ConveyorScript* found = GetScene()->GetConveyorAt(myPos.x, myPos.z);

        if (found && found != m_CurrentConveyor)
        {
            auto* foundTransform = found->GetComponent<TransformComponent>();
            if (foundTransform)
            {
                glm::vec3 foundPos = foundTransform->GetPosition();
                float dx = myPos.x - foundPos.x;
                float dz = myPos.z - foundPos.z;

                if (dx * dx + dz * dz < 0.25f) // 0.5f * 0.5f
                    m_CurrentConveyor = found;
            }
        }
        else if (found)
        {
            m_CurrentConveyor = found;
        }

        if (!m_CurrentConveyor) return;

        // 2. Logika fizyki taœmoci¹gu
        auto* conveyorTransform = m_CurrentConveyor->GetComponent<TransformComponent>();
        glm::vec3 convPos = conveyorTransform->GetPosition();

        float speed = m_CurrentConveyor->Speed * ts.GetSeconds();
        float diffX = convPos.x - myPos.x;
        float diffZ = convPos.z - myPos.z;

        // Zmienna bool, by œledziæ czy obiekt fizycznie siê poruszy³ w tej klatce
        bool moved = false;

        if (std::abs(m_CurrentConveyor->PushDirection.x) > 0.1f)
        {
            if (std::abs(diffZ) > 0.01f) {
                if (std::abs(diffZ) <= speed) myPos.z = convPos.z;
                else myPos.z += std::copysign(speed, diffZ);
            }
            else {
                myPos.x += m_CurrentConveyor->PushDirection.x * speed;
                myPos.z = convPos.z;
            }
            moved = true;
        }
        else if (std::abs(m_CurrentConveyor->PushDirection.z) > 0.1f)
        {
            if (std::abs(diffX) > 0.01f) {
                if (std::abs(diffX) <= speed) myPos.x = convPos.x;
                else myPos.x += std::copysign(speed, diffX);
            }
            else {
                myPos.z += m_CurrentConveyor->PushDirection.z * speed;
                myPos.x = convPos.x;
            }
            moved = true;
        }

        // 3. Wgrywamy now¹ pozycjê DOPIERO NA SAMYM KOÑCU
        // Dziêki temu flaga Dirty zapali siê tylko raz (jeœli obiekt faktycznie siê poruszy³)
        if (moved) {
            transform->SetPosition(myPos);
        }
    }
};