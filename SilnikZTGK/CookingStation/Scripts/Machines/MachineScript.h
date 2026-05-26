#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include "CookingStation/Scripts/Managers/IngredientType.h"
#include <vector>
#include <glm/glm.hpp>

class MachineScript : public ScriptableEntity
{
protected:
    float m_CookTime = 10.0f;
    float m_CurrentTime = 0.0f;
    float m_AutoDetectRadius = 3.0f;
    bool m_IsHeld = false;

public:
    std::vector<IngredientType> m_Ingredients;
    bool m_IsReady = false;
    bool m_IsAutomated = false;

    // NOWE: Zmienna, w której UI zapisuje nowo wyciągniętą maszynę
    static inline Entity PendingPickup = { std::numeric_limits<std::size_t>::max(), 0 };

    virtual void OnUpdate(Timestep ts) override
    {
        // NOWE: Jeśli to my jesteśmy świeżo wyciągniętą maszyną z UI, natychmiast się "podnosimy"
        if (PendingPickup.id != std::numeric_limits<std::size_t>::max() && PendingPickup.id == m_Entity.id)
        {
            m_IsHeld = true;
            PendingPickup = { std::numeric_limits<std::size_t>::max(), 0 }; // Czyścimy schowek
        }

        if (m_IsHeld)
        {
            auto* transform = GetComponent<TransformComponent>();
            if (!transform) return;

            glm::vec3 mouseWorldPos = GetMouseWorldPosition();
            transform->SetPosition(GridSystem::SnapToGrid(mouseWorldPos));

            if (Input::IsMouseButtonJustPressed(0))
            {
                if (!IsCellOccupied(transform->GetPosition()))
                {
                    m_IsHeld = false;
                    spdlog::info("Maszyna odlozona na siatke!");
                }
                else
                {
                    spdlog::warn("Nie mozesz tu postawic maszyny, pole zajete!");
                }
            }
            return;
        }
    }

    virtual void OnClick() override
    {
        if (!m_IsHeld && Input::IsKeyPressed(340)) 
        {
            m_IsHeld = true;
            spdlog::info("Podniesiono maszyne!");
        }
        else if (m_IsReady && !m_IsAutomated && !m_IsHeld)
        {
            TryTransferToPlate();
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
                if (tagSet->dense[i].Tag == "Plate" || tagSet->dense[i].Tag == "Talerz" || tagSet->dense[i].Tag == "Talerz_62")
                {
                    Entity plateEntity = tagSet->reverse[i];
                    auto* plateTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(plateEntity);

                    if (plateTransform)
                    {
                        // Pobieramy pozycj� talerza na siatce
                        glm::ivec2 plateCell = GridSystem::WorldToCell(plateTransform->GetPosition());

                        // Czy talerz jest o max 1 pole dalej w osi X i 1 w osi Y? (8 p�l dooko�a)
                        if (std::abs(myCell.x - plateCell.x) <= 1 && std::abs(myCell.y - plateCell.y) <= 1)
                        {
                            // Je�li jest kilka talerzy w s�siedztwie, wybierz fizycznie najbli�szy
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

};