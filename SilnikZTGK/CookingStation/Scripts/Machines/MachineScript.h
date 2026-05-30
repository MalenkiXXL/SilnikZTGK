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

    Entity m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
    Entity m_LastHighlightedPlate = { std::numeric_limits<std::size_t>::max(), 0 };

    bool m_IsMouseHoveringFood = false;

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
        ClearHighlight(); 
        GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityClickedEvent>(m_ClickSubId);
    }

    virtual void OnUpdate(Timestep ts) override
    {
        HandleFoodHover();

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

    Entity GetClosestAvailablePlate()
    {
        auto* myTransform = GetComponent<TransformComponent>();
        if (!myTransform) return { std::numeric_limits<std::size_t>::max(), 0 };

        // Jeœli poprzednio wybrany talerz nadal jest w zasiêgu, trzymaj siê go
        if (m_LastHighlightedPlate.id != std::numeric_limits<std::size_t>::max())
        {
            auto* lastTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_LastHighlightedPlate);
            if (lastTransform)
            {
                glm::ivec2 myCell = GridSystem::WorldToCell(myTransform->GetPosition());
                glm::ivec2 lastCell = GridSystem::WorldToCell(lastTransform->GetPosition());
                if (std::abs(myCell.x - lastCell.x) <= 1 && std::abs(myCell.y - lastCell.y) <= 1)
                    return m_LastHighlightedPlate; // nadal w zasiêgu, nie zmieniaj
            }
        }

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
        return closestPlate;
    }

    virtual void TryTransferToPlate()
    {
        Entity targetPlate = GetClosestAvailablePlate();

        if (targetPlate.id != std::numeric_limits<std::size_t>::max())
        {
            OnTransferToPlate(targetPlate);
            ResetMachineState();
        }
        else
        {
            spdlog::warn("Brak talerza na 8 sasiadujacych polach dookola maszyny!");
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

    // LOGIKA PODŒWIETLANIA TALERZA 
    void SetPlateHighlight(Entity plateEntity, bool state)
    {
        if (plateEntity.id == std::numeric_limits<std::size_t>::max()) return;

        auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(plateEntity);
        if (nsc)
        {
            for (auto& s : nsc->Scripts)
            {
                if (s.Name == "PlateScript" && s.Instance)
                {
                    static_cast<PlateScript*>(s.Instance)->SetHighlight(state);
                    break;
                }
            }
        }
    }

    void ClearHighlight()
    {
        if (m_LastHighlightedPlate.id != std::numeric_limits<std::size_t>::max())
        {
            SetPlateHighlight(m_LastHighlightedPlate, false);
            m_LastHighlightedPlate = { std::numeric_limits<std::size_t>::max(), 0 };
        }
    }

    virtual void HandleFoodHover()
    {
        if (m_IsHeld || !m_IsReady || m_IsAutomated)
        {
            if (m_IsMouseHoveringFood) {
                m_IsMouseHoveringFood = false;
                ClearHighlight();
            }
            return;
        }

        glm::vec3 targetPos = glm::vec3(0.0f);
        if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max())
        {
            auto* foodTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);
            if (foodTransform) targetPos = foodTransform->GetPosition();
        }
        else
        {
            auto* myTransform = GetComponent<TransformComponent>();
            if (myTransform) targetPos = myTransform->GetPosition();
        }

        glm::vec3 mousePos = GetMouseWorldPosition();
        glm::vec2 mousePos2D = { mousePos.x, mousePos.z };
        glm::vec2 targetPos2D = { targetPos.x, targetPos.z };

        float dist = glm::distance(mousePos2D, targetPos2D);
        bool isCurrentlyHovering = (dist < 1.0f);

        if (!isCurrentlyHovering)
        {
            if (m_IsMouseHoveringFood)
            {
                spdlog::info("DEBUG: Myszka ZJECHALA z dania.");
                m_IsMouseHoveringFood = false;
                ClearHighlight();
            }
            return; // <-- wyjdŸ wczeœniej, reszta logiki tylko gdy hover
        }

        // Od tu: isCurrentlyHovering == true
        if (!m_IsMouseHoveringFood)
        {
            spdlog::info("DEBUG: Myszka WJECHALA nad gotowe danie!");
            m_IsMouseHoveringFood = true;
        }

        Entity closestPlate = GetClosestAvailablePlate();

        // Aktualizuj podœwietlenie zawsze gdy kursor jest nad jedzeniem,
        // nie tylko gdy closestPlate siê zmieni³ — bo talerz móg³ siê pojawiæ
        if (closestPlate.id != m_LastHighlightedPlate.id)
        {
            ClearHighlight(); // gasi poprzedni

            if (closestPlate.id != std::numeric_limits<std::size_t>::max())
            {
                spdlog::info("DEBUG: Podswietlam talerz ID: {}", closestPlate.id);
                SetPlateHighlight(closestPlate, true);
            }
            else
            {
                spdlog::warn("DEBUG: Brak talerza w poblizu do podswietlenia.");
            }

            m_LastHighlightedPlate = closestPlate;
        }
    }
};