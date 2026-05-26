#pragma once
#include "CustomerScript.h" // DZIEDZICZYMY Z CUSTOMER SCRIPT!
#include "CookingStation/Core/Input.h"
#include "CookingStation/Scripts/MachineScript.h"
#include <glm/glm.hpp>
#include <limits>
#include <cmath>
#include <spdlog/spdlog.h>

class HelperCustomerScript : public CustomerScript
{
public:
    static inline bool IsAnyHelperDragged = false;

    float m_YOffset = 0.5f;
    float m_CellSize = 2.0f;
    float m_InteractRange = 1.5f;

    bool m_IsCarried = false;
    bool m_IsWorking = false;
    bool m_IsWaitingToHelp = false; 

    Entity m_AssignedMachine = { std::numeric_limits<std::size_t>::max(), 0 };
    glm::vec3 m_PositionBeforeDrag;

    // Nadpisujemy ReceiveFood - Pomocnik reaguje inaczej ni¿ zwyk³y klient
    void ReceiveFood(bool isCorrectOrder = true) override
    {
        IsServed = true;

        if (isCorrectOrder)
        {
            m_IsWaitingToHelp = true; // Teraz mo¿na go podnieœæ
            spdlog::info("Pomocnik nr {} zjadl i jest gotowy do pracy!", m_Entity.id);
            auto* tag = GetComponent<TagComponent>();
            if (tag) tag->Tag = "NajedzonyPomocnik";

            auto* transform = GetComponent<TransformComponent>();
            if (transform)
            {
                glm::vec3 pos = transform->GetPosition();
                transform->SetPosition(glm::vec3(pos.x, m_YOffset, pos.z));
            }
        }
        else
        {
            spdlog::info("Pomocnik nr {} dostal zly talerz! Obraza sie i znika.", m_Entity.id);
            auto* tf = GetComponent<TransformComponent>();
            if (tf) tf->SetPosition(glm::vec3(0.0f, -1000.0f, 0.0f));
        }
    }

    void OnUpdate(Timestep ts) override
    {
        // Jeœli jeszcze nie zjad³, siedzi i czeka
        if (!IsServed) return;

        // Logika podnoszenia i przenoszenia
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        glm::vec3 mousePos = GetMouseWorldPosition();
        glm::vec2 mousePos2D = { mousePos.x, mousePos.z };
        glm::vec2 myPos2D = { transform->GetPosition().x, transform->GetPosition().z };

        if (!m_IsCarried)
        {
            if (Input::IsMouseButtonJustPressed(0) && !IsAnyHelperDragged)
            {
                if (m_IsWaitingToHelp || m_IsWorking)
                {
                    if (glm::distance(mousePos2D, myPos2D) <= m_InteractRange)
                    {
                        PickUpHelper(transform);
                    }
                }
            }
        }
        else
        {
            float snappedX = std::floor(mousePos.x / m_CellSize) * m_CellSize + (m_CellSize / 2.0f);
            float snappedZ = std::floor(mousePos.z / m_CellSize) * m_CellSize + (m_CellSize / 2.0f);
            glm::vec3 snappedPos = { snappedX, m_YOffset + 0.5f, snappedZ };

            Entity hoveredMachine = GetHoveredMachine(mousePos);

            if (hoveredMachine.id != std::numeric_limits<std::size_t>::max())
            {
                snappedPos = GetClosestTileAroundMachine(hoveredMachine, mousePos);
            }

            transform->SetPosition(snappedPos);

            if (Input::IsMouseButtonJustPressed(0))
            {
                TryDropHelper(transform, hoveredMachine, snappedPos);
            }
            else if (Input::IsMouseButtonJustPressed(1))
            {
                CancelCarry(transform);
            }
        }
    }

private:
    void PickUpHelper(TransformComponent* transform)
    {
        m_IsCarried = true;
        IsAnyHelperDragged = true;
        m_PositionBeforeDrag = transform->GetPosition();
        m_IsWaitingToHelp = false;
        m_IsWorking = false;
        spdlog::info("Podniesiono Helpera!");
    }

    void TryDropHelper(TransformComponent* transform, Entity hoveredMachine, glm::vec3 dropPos)
    {
        m_IsCarried = false;
        IsAnyHelperDragged = false;

        dropPos.y = m_YOffset;
        transform->SetPosition(dropPos);

        if (hoveredMachine.id != std::numeric_limits<std::size_t>::max())
        {
            if (m_AssignedMachine.id != std::numeric_limits<std::size_t>::max())
            {
                auto* oldNsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_AssignedMachine);
                if (oldNsc)
                {
                    for (auto& scriptElement : oldNsc->Scripts)
                    {
                        if (auto* machine = dynamic_cast<MachineScript*>(scriptElement.Instance))
                        {
                            machine->m_IsAutomated = false;
                            spdlog::info("Odpieto helpera od starej maszyny - IsAutomated = false");
                            break;
                        }
                    }
                }
            }

            m_AssignedMachine = hoveredMachine;
            m_IsWorking = true;
            m_IsWaitingToHelp = false;

            auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(hoveredMachine);
            if (nsc)
            {
                for (auto& scriptElement : nsc->Scripts)
                {
                    if (auto* machine = dynamic_cast<MachineScript*>(scriptElement.Instance))
                    {
                        machine->m_IsAutomated = true;
                        spdlog::info("Helper przypisany - IsAutomated = true!");
                        break;
                    }
                }
            }

            auto* machineTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(hoveredMachine);
            if (machineTransform)
            {
                glm::vec3 dir = machineTransform->GetPosition() - dropPos;
                float angle = glm::degrees(std::atan2(dir.x, dir.z));
                transform->SetRotation({ 0.0f, angle + 180.0f, 0.0f });
            }
            spdlog::info("Helper przypisany do nowej maszyny!");
        }
        else
        {
            if (m_AssignedMachine.id != std::numeric_limits<std::size_t>::max())
            {
                auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_AssignedMachine);
                if (nsc)
                {
                    for (auto& scriptElement : nsc->Scripts)
                    {
                        if (auto* machine = dynamic_cast<MachineScript*>(scriptElement.Instance))
                        {
                            machine->m_IsAutomated = false;
                            spdlog::info("Helper odpieto od maszyny - IsAutomated = false");
                            break;
                        }
                    }
                }
            }

            m_AssignedMachine = { std::numeric_limits<std::size_t>::max(), 0 };
            m_IsWaitingToHelp = true;
            spdlog::info("Helper postawiony na ziemi, czeka na zadanie.");
        }
    }

    void CancelCarry(TransformComponent* transform)
    {
        m_IsCarried = false;
        IsAnyHelperDragged = false;
        transform->SetPosition(m_PositionBeforeDrag);
        spdlog::info("Anulowano przenoszenie Helpera.");
    }

    Entity GetHoveredMachine(glm::vec3 mousePos)
    {
        Entity closestMachine = { std::numeric_limits<std::size_t>::max(), 0 };
        float closestDist = 2.0f;

        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();

        if (scripts && transforms) {
            glm::vec2 mousePos2D = { mousePos.x, mousePos.z };

            for (size_t i = 0; i < scripts->dense.size(); ++i) {
                auto& nsc = scripts->dense[i];
                bool isMachine = false;

                for (auto& scriptElement : nsc.Scripts) {
                    if (scriptElement.Name == "PotScript" || scriptElement.Name == "CuttingBoardScript") {
                        isMachine = true;
                        break;
                    }
                }

                if (isMachine) {
                    Entity machineEntity = scripts->reverse[i];
                    auto* machineTransform = transforms->Get(machineEntity);

                    if (machineTransform) {
                        glm::vec2 machinePos2D = { machineTransform->GetPosition().x, machineTransform->GetPosition().z };
                        float dist = glm::distance(mousePos2D, machinePos2D);

                        if (dist < closestDist) {
                            closestDist = dist;
                            closestMachine = machineEntity;
                        }
                    }
                }
            }
        }
        return closestMachine;
    }

    glm::vec3 GetClosestTileAroundMachine(Entity machine, glm::vec3 mousePos)
    {
        auto* machineTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(machine);
        if (!machineTransform) return mousePos;

        glm::vec3 mPos = machineTransform->GetPosition();

        float offsetX = (mousePos.x > mPos.x) ? m_CellSize : -m_CellSize;
        float offsetZ = (mousePos.z > mPos.z) ? m_CellSize : -m_CellSize;

        if (std::abs(mousePos.x - mPos.x) > std::abs(mousePos.z - mPos.z)) {
            return glm::vec3(mPos.x + offsetX, m_YOffset + 0.5f, mPos.z);
        }
        else {
            return glm::vec3(mPos.x, m_YOffset + 0.5f, mPos.z + offsetZ);
        }
    }
};