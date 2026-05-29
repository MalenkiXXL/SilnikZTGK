#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include "CookingStation/Scripts/Managers/IngredientType.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Scripts/PlateScript.h"
#include <vector>
#include <glm/glm.hpp>

class MachineScript : public ScriptableEntity
{
protected:
    float m_CookTime = 10.0f;
    float m_CurrentTime = 0.0f;
    float m_AutoDetectRadius = 3.0f;
    bool m_IsHeld = false;
    std::size_t m_ClickSubId = 0;
    glm::vec3 m_OriginalPosition = glm::vec3(0.0f);
    bool m_IsNewlySpawned = false;
    float m_PickupDelay = 0.0f;

public:
    std::vector<IngredientType> m_Ingredients;
    bool m_IsReady = false;
    bool m_IsAutomated = false;

    static inline Entity PendingPickup = { std::numeric_limits<std::size_t>::max(), 0 };
    static inline bool GlobalIsMachineHeld = false;
    static inline bool GlobalIsHoveringUI = false;

    virtual void OnCreate() override
    {
        m_ClickSubId = GetScene()->GetWorld().GetEventBus().Subscribe<EntityClickedEvent>(
            [this](const EntityClickedEvent& e) {
                if (e.TargetEntity.id == m_Entity.id)
                {
                    this->HandleClick();
                }
            }
        );
    }

    virtual void OnDestroy() override
    {
        GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityClickedEvent>(m_ClickSubId);
    }

    virtual void OnUpdate(Timestep ts) override
    {
        if (PendingPickup.id != std::numeric_limits<std::size_t>::max() && PendingPickup.id == m_Entity.id)
        {
            m_IsHeld = true;
            m_IsNewlySpawned = true;
            GlobalIsMachineHeld = true;
            m_PickupDelay = 0.2f;
            PendingPickup = { std::numeric_limits<std::size_t>::max(), 0 };
        }

        if (m_IsHeld)
        {
            if (m_PickupDelay > 0.0f)
            {
                m_PickupDelay -= (float)ts;
            }

            auto* transform = GetComponent<TransformComponent>();
            if (!transform) return;

            float originalY = transform->GetPosition().y;

            glm::vec3 mouseWorldPos = GetMouseWorldPosition();
            glm::vec3 snappedPos = GridSystem::SnapToGrid(mouseWorldPos);

            snappedPos.y = originalY;
            transform->SetPosition(snappedPos);

            if (Input::IsMouseButtonJustPressed(0) && m_PickupDelay <= 0.0f)
            {
                if (!GlobalIsHoveringUI && !IsCellOccupied(transform->GetPosition()))
                {
                    m_IsHeld = false;
                    m_IsNewlySpawned = false;
                    GlobalIsMachineHeld = false;
                }
            }
            else if (Input::IsMouseButtonJustPressed(1))
            {
                if (m_IsNewlySpawned)
                {
                    m_IsHeld = false;
                    GlobalIsMachineHeld = false;
                    GetScene()->DestroyEntity(m_Entity);
                    return;
                }
                else
                {
                    transform->SetPosition(m_OriginalPosition);
                    m_IsHeld = false;
                    GlobalIsMachineHeld = false;
                }
            }
            return;
        }
    }

    virtual bool AddIngredient(IngredientType type)
    {
        if (m_IsReady || m_Ingredients.size() >= 2) return false;
        m_Ingredients.push_back(type);
        m_IsReady = false;
        m_CurrentTime = 0.0f;
        UpdateVisuals();
        return true;
    }

    virtual void HandleClick()
    {
        if (!m_IsHeld && Input::IsKeyPressed(340) && !GlobalIsMachineHeld)
        {
            m_IsHeld = true;
            m_IsNewlySpawned = false;
            GlobalIsMachineHeld = true;
            m_PickupDelay = 0.2f;
            auto* transform = GetComponent<TransformComponent>();
            if (transform) m_OriginalPosition = transform->GetPosition();
        }
        else if (m_IsReady && !m_IsAutomated && !m_IsHeld)
        {
            TryTransferToPlate();
        }
    }

protected:
    bool IsCellOccupied(const glm::vec3& targetPos)
    {
        glm::ivec2 targetCell = GridSystem::WorldToCell(targetPos);
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();
        if (!transforms) return false;

        for (size_t i = 0; i < transforms->dense.size(); ++i)
        {
            Entity otherEntity = transforms->reverse[i];
            if (otherEntity.id == m_Entity.id) continue;

            glm::ivec2 otherCell = GridSystem::WorldToCell(transforms->dense[i].GetPosition());
            if (targetCell == otherCell)
            {
                return true;
            }
        }
        return false;
    }

    virtual void TryTransferToPlate()
    {
        auto* myTransform = GetComponent<TransformComponent>();
        if (!myTransform) return;

        Entity closestPlate = { std::numeric_limits<std::size_t>::max(), 0 };
        float closestDist = 999.0f;
        glm::ivec2 myCell = GridSystem::WorldToCell(myTransform->GetPosition());

        auto* tagSet = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        if (tagSet)
        {
            for (size_t i = 0; i < tagSet->dense.size(); ++i)
            {
                const std::string& tagName = tagSet->dense[i].Tag;

                if (tagName.find("Plate") != std::string::npos || tagName.find("Talerz") != std::string::npos)
                {
                    Entity plateEntity = tagSet->reverse[i];
                    bool isOccupied = false;

                    auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(plateEntity);
                    if (nsc)
                    {
                        for (auto& script : nsc->Scripts)
                        {
                            if (script.Name == "PlateScript" && script.Instance)
                            {
                                auto* plateScript = static_cast<PlateScript*>(script.Instance);
                                if (!plateScript->m_Ingredients.empty() || plateScript->m_CompletedDish != IngredientType::None)
                                {
                                    isOccupied = true;
                                }
                                break;
                            }
                        }
                    }

                    if (!isOccupied)
                    {
                        for (size_t j = 0; j < tagSet->dense.size(); ++j)
                        {
                            if (tagSet->dense[j].Tag == "UgotowaneDanie")
                            {
                                Entity childEntity = tagSet->reverse[j];
                                if (GetScene()->GetParent(childEntity).id == plateEntity.id)
                                {
                                    isOccupied = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (isOccupied)
                    {
                        continue;
                    }

                    auto* plateTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(plateEntity);

                    if (plateTransform)
                    {
                        glm::ivec2 plateCell = GridSystem::WorldToCell(plateTransform->GetPosition());

                        if (std::abs(myCell.x - plateCell.x) <= 1 && std::abs(myCell.y - plateCell.y) <= 1)
                        {
                            float dist = glm::distance(myTransform->GetPosition(), plateTransform->GetPosition());
                            if (dist < closestDist)
                            {
                                closestDist = dist;
                                closestPlate = plateEntity;
                            }
                        }
                    }
                }
            }
        }

        if (closestPlate.id != std::numeric_limits<std::size_t>::max())
        {
            OnTransferToPlate(closestPlate);
            ResetMachineState();
        }
    }

    virtual void OnTransferToPlate(Entity plateEntity) {}

    virtual void ResetMachineState()
    {
        m_IsReady = false;
        m_CurrentTime = 0.0f;
        m_Ingredients.clear();
        UpdateVisuals();
    }

    virtual void UpdateVisuals() = 0;

};