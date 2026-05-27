#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scripts/ConveyorBelt/ConveyorScript.h"
#include <cmath>

class ItemScript : public ScriptableEntity
{
    glm::vec3 m_TargetPosition = { 0.0f, 0.0f, 0.0f };
    bool m_IsMoving = false;
    float m_GridSize = 2.0f; // Rozmiar kratki
    float m_CurrentSpeed = 2.0f;

    ConveyorScript* m_CurrentConveyor = nullptr;
    ConveyorScript* m_TargetConveyor = nullptr;

public:
    void OnCreate() override {}

    void OnDestroy() override
    {
        // 1. Zwalniamy taśmę, na której staliśmy
        if (m_CurrentConveyor)
        {
            m_CurrentConveyor->IsOccupied = false;
            m_CurrentConveyor->IsJammed = false;
        }

        // 2. Zwalniamy taśmę, na którą właśnie wjeżdżaliśmy
        if (m_TargetConveyor)
        {
            m_TargetConveyor->IsOccupied = false;
            m_TargetConveyor->IsJammed = false;
        }

        // Zabezpieczenie pointerów
        m_CurrentConveyor = nullptr;
        m_TargetConveyor = nullptr;
    }
    
    void ReleaseConveyors()
    {
        if (m_CurrentConveyor) {
            m_CurrentConveyor->IsOccupied = false;
            m_CurrentConveyor->IsJammed = false;
            m_CurrentConveyor = nullptr;
        }
        if (m_TargetConveyor) {
            m_TargetConveyor->IsOccupied = false;
            m_TargetConveyor->IsJammed = false;
            m_TargetConveyor = nullptr;
        }
        m_IsMoving = false;
    }

    void OnUpdate(Timestep ts) override
    {
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        glm::vec3 myPos = transform->GetPosition();

        float distanceToMove = m_CurrentSpeed * ts.GetSeconds();
        bool movedThisFrame = false;

        if (!m_IsMoving)
        {
            if (!FindNextTarget(myPos))
            {
                if (m_CurrentConveyor) m_CurrentConveyor->IsJammed = true;
                return;
            }
            else
            {
                if (m_CurrentConveyor) m_CurrentConveyor->IsJammed = false;
            }
        }

        while (distanceToMove > 0.001f && m_IsMoving)
        {
            glm::vec3 diff = m_TargetPosition - myPos;
            diff.y = 0.0f;
            float distanceToTarget = glm::length(diff);

            if (distanceToTarget <= distanceToMove)
            {
                myPos.x = m_TargetPosition.x;
                myPos.z = m_TargetPosition.z;

                distanceToMove -= distanceToTarget;

                if (m_CurrentConveyor)
                {
                    m_CurrentConveyor->IsOccupied = false;
                    m_CurrentConveyor->IsJammed = false;
                }

                m_CurrentConveyor = m_TargetConveyor;

                if (!FindNextTarget(myPos))
                {
                    m_IsMoving = false;
                    if (m_CurrentConveyor) m_CurrentConveyor->IsJammed = true;
                }
            }
            else
            {
                glm::vec3 dir = diff / distanceToTarget;
                myPos.x += dir.x * distanceToMove;
                myPos.z += dir.z * distanceToMove;

                distanceToMove = 0.0f;
            }

            movedThisFrame = true;
        }

        if (movedThisFrame)
        {
            transform->SetPosition(myPos);
        }
    }

private:
    bool FindNextTarget(glm::vec3 currentPos)
    {
        ConveyorScript* currentConveyor = GetScene()->GetConveyorAt(currentPos.x, currentPos.z);
        if (!currentConveyor) return false;

        if (!m_CurrentConveyor) {
            m_CurrentConveyor = currentConveyor;
            m_CurrentConveyor->IsOccupied = true;
        }

        glm::vec3 nextPos = currentPos + (currentConveyor->PushDirection * m_GridSize);
        ConveyorScript* nextConveyor = GetScene()->GetConveyorAt(nextPos.x, nextPos.z);
        
        if (nextConveyor)
        {
            if (!nextConveyor->IsOccupied)
            {
                nextConveyor->IsOccupied = true;

                m_TargetConveyor = nextConveyor;

                m_TargetPosition = nextPos;
                m_IsMoving = true;

                return true;
            }
        }

        return false;
    }
};