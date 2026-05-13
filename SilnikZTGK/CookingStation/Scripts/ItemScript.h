#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scripts/ConveyorScript.h"
#include <cmath>

class ItemScript : public ScriptableEntity
{
    glm::vec3 m_TargetPosition = { 0.0f, 0.0f, 0.0f };
    bool m_IsMoving = false;
    float m_GridSize = 2.0f; // Rozmiar kratki
    float m_CurrentSpeed = 2.0f;

public:
    void OnCreate() override {}

    void OnUpdate(Timestep ts) override
    {
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        glm::vec3 myPos = transform->GetPosition();

        // Ca≥kowity dystans, jaki mamy do pokonania w TEJ klatce
        float distanceToMove = m_CurrentSpeed * ts.GetSeconds();
        bool movedThisFrame = false;

        // 1. Jeúli obiekt stoi, sprawdzamy, czy w ogÛle ma po czym jechaÊ
        if (!m_IsMoving)
        {
            if (!FindNextTarget(myPos))
                return; // Nic pod nami nie ma, stoimy w miejscu
        }

        // 2. PÍtla ruchu. Jeúli dotrzemy do celu szybciej niø "distanceToMove" 
        // to resztÍ ruchu wykorzystujemy na p≥ynny wjazd na kolejnπ taúmÍ
        while (distanceToMove > 0.001f && m_IsMoving)
        {
            glm::vec3 diff = m_TargetPosition - myPos;
            diff.y = 0.0f; // Ignorujemy oú Y, jeüdzimy tylko w poziomie
            float distanceToTarget = glm::length(diff);

            if (distanceToTarget <= distanceToMove)
            {
                // Dotarliúmy dok≥adnie do úrodka kratki
                myPos.x = m_TargetPosition.x;
                myPos.z = m_TargetPosition.z;

                // Odejmujemy dystans, ktÛry zuøyliúmy na dojazd do úrodka
                distanceToMove -= distanceToTarget;

                // OD RAZU szukamy nowej taúmy, bez czekania na nowπ klatkÍ!
                if (!FindNextTarget(myPos))
                {
                    m_IsMoving = false; // Koniec trasy, zatrzymujemy siÍ na úrodku
                }
            }
            else
            {
                // Zwyk≥y ruch w stronÍ celu (zosta≥o nam wiÍcej drogi niø dystansu do úrodka)
                glm::vec3 dir = diff / distanceToTarget;
                myPos.x += dir.x * distanceToMove;
                myPos.z += dir.z * distanceToMove;

                // Zuøyliúmy ca≥y ruch z tej klatki
                distanceToMove = 0.0f;
            }

            movedThisFrame = true;
        }

        // 3. Aktualizacja pozycji tylko wtedy, gdy faktycznie siÍ przemieúciliúmy (Dirty Flag)
        if (movedThisFrame)
        {
            transform->SetPosition(myPos);
        }
    }

private:
    // Pomocnicza funkcja do wy≥apywania kolejnej taúmy i ustawiania celu
    bool FindNextTarget(glm::vec3 currentPos)
    {
        ConveyorScript* currentConveyor = GetScene()->GetConveyorAt(currentPos.x, currentPos.z);

        if (currentConveyor)
        {
            m_CurrentSpeed = currentConveyor->Speed;
            auto* conveyorTransform = currentConveyor->GetComponent<TransformComponent>();

            if (conveyorTransform)
            {
                glm::vec3 convPos = conveyorTransform->GetPosition();

                // årodek kolejnej kratki, zgodnie z kierunkiem taúmy
                m_TargetPosition = convPos + (currentConveyor->PushDirection * m_GridSize);
                m_IsMoving = true;
                return true;
            }
        }

        return false;
    }
};