#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scripts/ConveyorBelt/ConveyorScript.h"
#include "CookingStation/Events/GameEvents.h" 
#include <cmath>

class ItemScript : public ScriptableEntity
{
    glm::vec3 m_TargetPosition = { 0.0f, 0.0f, 0.0f };
    bool m_IsMoving = false;
    float m_GridSize = 2.0f;
    float m_CurrentSpeed = 2.0f;

    ConveyorScript* m_CurrentConveyor = nullptr;
    ConveyorScript* m_TargetConveyor = nullptr;

    std::size_t m_GrabbedSubId = 0;
    std::size_t m_ClickSubId = 0;

public:
    void OnCreate() override
    {
        auto& bus = GetScene()->GetWorld().GetEventBus();
        m_GrabbedSubId = bus.Subscribe<PlateGrabbedEvent>([this](const PlateGrabbedEvent& e) {
            if (e.Plate.id == m_Entity.id) {
                this->ReleaseConveyors();
            }
            });

        m_ClickSubId = bus.Subscribe<EntityClickedEvent>([this](const EntityClickedEvent& e) {
            if (e.TargetEntity.id == m_Entity.id) {
                if (!Input::IsUICapturingMouse()) {
                    // Opcjonalna obsługa kliknięcia w przedmiot na taśmie
                }
            }
            });
    }

    void OnDestroy() override
    {
        ReleaseConveyors();
        auto* scene = GetScene();
        if (scene) {
            scene->GetWorld().GetEventBus().Unsubscribe<PlateGrabbedEvent>(m_GrabbedSubId);
            scene->GetWorld().GetEventBus().Unsubscribe<EntityClickedEvent>(m_ClickSubId);
        }
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
    }

    void OnUpdate(Timestep ts) override
    {
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        glm::vec3 myPos = transform->GetPosition();

        if (!m_IsMoving)
        {
            if (FindNextTarget(myPos))
            {
                m_IsMoving = true;
                if (m_CurrentConveyor) m_CurrentConveyor->IsJammed = false;
            }
            else
            {
                // Przedmiot fizycznie stoi i czeka
                if (m_CurrentConveyor) m_CurrentConveyor->IsJammed = true;
            }
        }

        bool movedThisFrame = false;

        if (m_IsMoving)
        {
            float distanceToMove = m_CurrentSpeed * ts.GetSeconds();
            glm::vec3 diff = m_TargetPosition - myPos;
            float distanceToTarget = glm::length(glm::vec2(diff.x, diff.z));

            if (distanceToTarget <= distanceToMove)
            {
                myPos = m_TargetPosition;

                if (m_CurrentConveyor) {
                    m_CurrentConveyor->IsOccupied = false;
                    m_CurrentConveyor->IsJammed = false;
                }

                m_CurrentConveyor = m_TargetConveyor;
                m_TargetConveyor = nullptr;

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
        // 1. Zabezpieczenie: jeśli pojawiliśmy się na taśmie (np. zrzucono nas)
        if (!m_CurrentConveyor) {
            m_CurrentConveyor = GetScene()->GetConveyorAt(currentPos.x, currentPos.z);
            if (m_CurrentConveyor) m_CurrentConveyor->IsOccupied = true;
            else return false;
        }

        glm::vec3 nextPos = currentPos + (m_CurrentConveyor->PushDirection * m_GridSize);
        ConveyorScript* nextConveyor = GetScene()->GetConveyorAt(nextPos.x, nextPos.z);

        if (nextConveyor)
        {
            // 2. Jeśli taśma melduje się jako wolna...
            if (!nextConveyor->IsOccupied)
            {
                // 3. FIZYCZNY RADAR - Ostatecznie sprawdzamy, czy nic tam fizycznie nie stoi!
                bool isPhysicallyClear = true;
                auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();
                auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();

                if (transforms && tags) {
                    for (size_t i = 0; i < transforms->dense.size(); ++i) {
                        Entity e = transforms->reverse[i];
                        if (e.id == m_Entity.id) continue;

                        glm::vec3 otherPos = transforms->dense[i].GetPosition();

                        // Skanujemy obszar przyszłego taśmociągu (i odrobinę przed nami)
                        if (glm::distance(glm::vec2(otherPos.x, otherPos.z), glm::vec2(nextPos.x, nextPos.z)) < 1.4f)
                        {
                            auto* tag = tags->Get(e);
                            if (tag && (tag->Tag.find("BeltItem") != std::string::npos || tag->Tag.find("Plate") != std::string::npos)) {
                                isPhysicallyClear = false;
                                break;
                            }
                        }
                    }
                }

                // Wjeżdżamy tylko jeśli nowa taśma jest CAŁKOWICIE w 100% PUSTA
                if (isPhysicallyClear)
                {
                    nextConveyor->IsOccupied = true;
                    m_TargetConveyor = nextConveyor;
                    m_TargetPosition = nextPos;
                    return true;
                }
            }
        }

        return false;
    }
};