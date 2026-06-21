#pragma once
#include "CustomerScript.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include <glm/glm.hpp>
#include <limits>
#include <cmath>

class HelperCustomerScript : public CustomerScript
{
public:
public:
    float m_YOffset = 0.5f;
    Entity m_AssignedMachine = { std::numeric_limits<std::size_t>::max(), 0 };
    glm::vec3 m_HighlightColor = glm::vec3(0.2f, 0.8f, 0.2f);
    float m_RotationOffset = -90.0f;

    void OnCreate() override
    {
        CustomerScript::OnCreate();

        auto* tagComp = GetComponent<TagComponent>();
        if (tagComp) {
            if (tagComp->Tag.find("Marchewka") != std::string::npos) {
                m_HighlightColor = glm::vec3(0.3f, 0.4f, 0.71f);
            }
            else if (tagComp->Tag.find("Pomidor") != std::string::npos) {
                m_HighlightColor = glm::vec3(0.94f, 0.31f, 0.47f);
            }
            else if (tagComp->Tag.find("Rzodkiewka") != std::string::npos) {
                m_HighlightColor = glm::vec3(0.66f, 0.52f, 0.95f);
                m_RotationOffset = 90.0f;
            }
        }
    }

    void OnDestroy() override
    {
        if (m_AssignedMachine.id != std::numeric_limits<std::size_t>::max()) {
            SetMachineAutomated(m_AssignedMachine, false);
        }
        CustomerScript::OnDestroy();
    }

    void ReceiveFood(bool isCorrectOrder = true) override
    {
        IsServed = true;

        if (m_ReceivedFood.id != std::numeric_limits<std::size_t>::max()) {
            GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_ReceivedFood });
            m_ReceivedFood = { std::numeric_limits<std::size_t>::max(), 0 };
        }

        if (isCorrectOrder)
        {
            auto* tag = GetComponent<TagComponent>();
            if (tag) tag->Tag = "NajedzonyPomocnik";

            TeleportToWaitingArea();
        }
        else
        {
            GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_Entity });
        }
    }

    void OnUpdate(Timestep ts) override
    {
        if (!IsServed) return;

        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        Entity closestMachine = FindAdjacentMachine(transform->GetPosition());

        if (closestMachine.id != m_AssignedMachine.id) {
            if (m_AssignedMachine.id != std::numeric_limits<std::size_t>::max()) {
                SetMachineAutomated(m_AssignedMachine, false);
            }

            m_AssignedMachine = closestMachine;

            if (m_AssignedMachine.id != std::numeric_limits<std::size_t>::max()) {
                SetMachineAutomated(m_AssignedMachine, true);
                RotateTowardsMachine(transform, m_AssignedMachine);

                glm::vec3 highlightColor = m_HighlightColor;

                GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                    m_Entity, highlightColor, 0.8f, false
                    });

                GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                    m_AssignedMachine, highlightColor, 0.8f, false
                    });
            }
        }
        else if (m_AssignedMachine.id != std::numeric_limits<std::size_t>::max()) {
            RotateTowardsMachine(transform, m_AssignedMachine);
        }
    }

private:
    void TeleportToWaitingArea()
    {
        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();

        if (!scripts || !transforms) return;

        glm::vec3 targetPos = { -5.0f, 0.0f, 11.0f };

        for (size_t i = 0; i < scripts->dense.size(); ++i) {
            auto& nsc = scripts->dense[i];
            bool foundCrate = false;

            for (auto& s : nsc.Scripts) {
                if (s.Name == "CrateScript") {
                    Entity crateEntity = scripts->reverse[i];
                    auto* crateTf = transforms->Get(crateEntity);

                    if (crateTf) {
                        glm::vec3 cratePos = crateTf->GetPosition();

                        float offsetX = (m_Entity.id % 3) * 1.5f - 1.5f;
                        float offsetZ = (m_Entity.id % 2) * 1.0f + 2.5f;

                        targetPos = glm::vec3(cratePos.x + offsetX, 0.0f, cratePos.z + offsetZ);
                        foundCrate = true;
                        break;
                    }
                }
            }
            if (foundCrate) break;
        }

        auto* myTransform = GetComponent<TransformComponent>();
        if (myTransform) {
            myTransform->SetPosition(targetPos);
            myTransform->SetRotation({ 0.0f, 180.0f, 0.0f });
        }
    }

    Entity FindAdjacentMachine(glm::vec3 myPos)
    {
        Entity foundMachine = { std::numeric_limits<std::size_t>::max(), 0 };
        float closestDist = 999.0f;

        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();
        if (!scripts || !transforms) return foundMachine;

        glm::vec2 myPos2D = { myPos.x, myPos.z };

        for (size_t i = 0; i < scripts->dense.size(); ++i) {
            auto& nsc = scripts->dense[i];
            bool isMachine = false;

            for (auto& s : nsc.Scripts) {
                if (s.Name == "PotScript" || s.Name == "CuttingBoardScript" || s.Name == "MixerScript" || s.Name == "OvenScript" || s.Name == "PanScript") {
                    isMachine = true;
                    break;
                }
            }

            if (isMachine) {
                Entity machEnt = scripts->reverse[i];
                auto* machTf = transforms->Get(machEnt);
                if (machTf) {
                    glm::vec2 machPos2D = { machTf->GetPosition().x, machTf->GetPosition().z };
                    float dist = glm::distance(myPos2D, machPos2D);

                    if (dist <= 2.2f && dist < closestDist) {
                        closestDist = dist;
                        foundMachine = machEnt;
                    }
                }
            }
        }
        return foundMachine;
    }

    void SetMachineAutomated(Entity machineEnt, bool state)
    {
        auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(machineEnt);
        if (nsc) {
            for (auto& scriptElement : nsc->Scripts) {
                if (auto* machine = dynamic_cast<MachineScript*>(scriptElement.Instance)) {
                    machine->m_IsAutomated = state;
                    break;
                }
            }
        }
    }

    void RotateTowardsMachine(TransformComponent* myTf, Entity machineEnt)
    {
        auto* machTf = GetScene()->GetWorld().GetComponent<TransformComponent>(machineEnt);
        if (machTf) {
            glm::vec3 dir = machTf->GetPosition() - myTf->GetPosition();
            if (glm::length(dir) > 0.01f) {
                float angle = glm::degrees(std::atan2(dir.x, dir.z));
                myTf->SetRotation({ 0.0f, angle + m_RotationOffset, 0.0f });
            }
        }
    }
};