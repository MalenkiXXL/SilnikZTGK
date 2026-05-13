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

        // 1. Pobieramy obecn¹ pozycjê obiektu
        glm::vec3 myPos = transform->GetPosition();

        if (!m_IsMoving)
        {
            ConveyorScript* currentConveyor = GetScene()->GetConveyorAt(myPos.x, myPos.z);


            if (currentConveyor)
            {
                // Zapisujemy prêdkoœæ z taœmy, na której w³aœnie jesteœmy
                m_CurrentSpeed = currentConveyor->Speed;

                auto* conveyorTransform = currentConveyor->GetComponent<TransformComponent>();
                if (conveyorTransform)
                {
                    glm::vec3 convPos = conveyorTransform->GetPosition();

                    // Obliczamy œrodek KOLEJNEJ kratki
                    m_TargetPosition = convPos + (currentConveyor->PushDirection * m_GridSize);

                    // Wyrównanie obiektu do œrodka obecnej taœmy
                    myPos.x = convPos.x;
                    myPos.z = convPos.z;

                    m_IsMoving = true;
                }
            }
        }

        if (m_IsMoving)
        {
            float step = m_CurrentSpeed * ts.GetSeconds();

            // Wektor ró¿nicy miêdzy celem a nasz¹ pozycj¹ (tylko na osiach X i Z)
            glm::vec3 diff = m_TargetPosition - myPos;
            diff.y = 0.0f;

            float distance = glm::length(diff);

            // Sprawdzamy, czy w tej klatce przekroczymy/osi¹gniemy cel
            if (distance <= step)
            {
                // Dotarliœmy równiutko do celu
                myPos.x = m_TargetPosition.x;
                myPos.z = m_TargetPosition.z;

                // Zatrzymujemy ruch. W kolejnej klatce skrypt sprawdzi now¹ taœmê pod now¹ pozycj¹.
                m_IsMoving = false;
            }
            else
            {
                // Jeœli cel jest jeszcze daleko, po prostu przemieszczamy siê o wartoœæ 'step'
                glm::vec3 dir = diff / distance;
                myPos.x += dir.x * step;
                myPos.z += dir.z * step;
            }

            // 3. Wgrywamy now¹ pozycjê DOPIERO NA SAMYM KOÑCU
            transform->SetPosition(myPos);
        }
    }
};