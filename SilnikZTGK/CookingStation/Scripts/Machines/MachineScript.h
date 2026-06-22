#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include "CookingStation/Scripts/Managers/IngredientType.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Scripts/PlateScript.h"
#include "CookingStation/Core/AudioEngine.h"
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

    std::size_t m_FoodClickSubId = 0;
    std::size_t m_HoverSubId = 0;
    bool m_IsHoveredThisFrame = false;

public:

    virtual void TryTransferToPlate()
    {
        Entity targetPlate = GetClosestAvailablePlate();

        if (targetPlate.id != std::numeric_limits<std::size_t>::max())
        {
            ClearHighlight();

            // Zachowujemy stare ID przed próbą przekazania
            Entity foodBeforeTransfer = m_SpawnedFood;

            OnTransferToPlate(targetPlate);

            // Jeśli m_SpawnedFood się zmieniło (zostało zresetowane w PlaceSpawnedFoodOnPlate), to znaczy, że transfer się udał!
            if (m_SpawnedFood.id != foodBeforeTransfer.id)
            {
                ResetMachineState();
            }
        }
        else
        {
            spdlog::warn("Brak talerza w bezpiecznym promieniu od maszyny!");
        }
    }

    std::vector<IngredientType> m_Ingredients;
    bool m_IsReady = false;
    bool m_IsAutomated = false;

    static inline Entity PendingPickup = { std::numeric_limits<std::size_t>::max(), 0 };
    static inline bool GlobalIsMachineHeld = false;

    virtual void OnCreate() override
    {
        m_ClickSubId = GetScene()->GetWorld().GetEventBus().Subscribe<EntityClickedEvent>(
            [this](const EntityClickedEvent& e) {
                if (e.TargetEntity.id == m_Entity.id)
                    this->HandleClick();
            }
        );

        m_FoodClickSubId = GetScene()->GetWorld().GetEventBus().Subscribe<EntityClickedEvent>(
            [this](const EntityClickedEvent& e) {
                if (m_SpawnedFood.id == std::numeric_limits<std::size_t>::max()) return;
                if (e.TargetEntity.id != m_SpawnedFood.id) return;
                if (!m_IsReady || m_IsAutomated || m_IsHeld) return;

                TryTransferToPlate();
            }
        );

        m_HoverSubId = GetScene()->GetWorld().GetEventBus().Subscribe<EntityHoveredEvent>(
            [this](const EntityHoveredEvent& e) {
                if (m_IsHeld || !m_IsReady || m_IsAutomated) {
                    if (m_IsMouseHoveringFood) {
                        m_IsMouseHoveringFood = false;
                        ClearHighlight();
                    }
                    return;
                }
                if (m_SpawnedFood.id == std::numeric_limits<std::size_t>::max()) return;

                bool isHoveringFood = (e.TargetEntity.id == m_SpawnedFood.id);

                if (isHoveringFood)
                {
                    m_IsMouseHoveringFood = true;

                    Entity closestPlate = GetClosestAvailablePlate();

                        if (closestPlate.id != m_LastHighlightedPlate.id)
                        {
                            ClearHighlight();
                            m_LastHighlightedPlate = closestPlate;
                        }

                        if (m_LastHighlightedPlate.id != std::numeric_limits<std::size_t>::max()) {
                            SetPlateHighlight(m_LastHighlightedPlate, true);
                        }
                    }
                    else if (!isHoveringFood && m_IsMouseHoveringFood)
                    {
                        m_IsMouseHoveringFood = false;
                        ClearHighlight();
                    }
                }
        );
    }

    virtual void OnDestroy() override
    {
        ClearHighlight();
        GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityClickedEvent>(m_ClickSubId);
        GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityClickedEvent>(m_FoodClickSubId);
        GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityHoveredEvent>(m_HoverSubId);
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
                if (!Input::IsUICapturingMouse() && !IsCellOccupied(transform->GetPosition())) {
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
        if (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0))
        {
            if (m_IsReady && !m_IsAutomated && m_SpawnedFood.id != std::numeric_limits<std::size_t>::max())
            {
                auto* foodTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);
                if (foodTf)
                {
                    glm::vec2 cursor2D = { GetMouseWorldPosition().x, GetMouseWorldPosition().z };
                    glm::vec2 food2D = { foodTf->GetPosition().x, foodTf->GetPosition().z };

                    if (glm::distance(cursor2D, food2D) < 1.5f) {
                        TryTransferToPlate();
                    }
                }
            }
        }
    }

    virtual bool AddIngredient(IngredientType type)
    {
        if (m_IsReady || m_Ingredients.size() >= 2){
            AudioEngine::Play("CookingStation/Assets/sounds/put_in_pot.mp3");
            return false;
        } else {
            m_Ingredients.push_back(type);
            m_IsReady = false;
            m_CurrentTime = 0.0f;
            UpdateVisuals();
            AudioEngine::Play("CookingStation/Assets/sounds/cooking.mp3");
            return true;
        }
    }

    void OnHoverCursor() override
    {
        m_IsHoveredThisFrame = true;

        if (m_IsReady && !m_IsAutomated && !GlobalIsMachineHeld && !m_IsHeld)
        {
            Entity closestPlate = GetClosestAvailablePlate();
            if (closestPlate.id != std::numeric_limits<std::size_t>::max())
            {
                m_LastHighlightedPlate = closestPlate;
                SetPlateHighlight(closestPlate, true);
            }
        }
    }

    virtual bool CanAcceptIngredient(IngredientType type)
    {
        if (m_IsReady || m_Ingredients.size() >= 2) return false;

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
        }else
        {
            if (m_IsReady && !m_IsAutomated)
            {
                TryTransferToPlate();
            }
        }
    }

protected:

    Entity SpawnMachineFood(IngredientType type, const std::string& tag)
    {
        auto builder = GetScene()->GetWorld().BuildEntity();
        if (!tag.empty()) builder.With<TagComponent>({ tag });

        TransformComponent tc;
        IngredientMetadata meta = GetIngredientMetadata(type);

        tc.SetScale(meta.scale);
        tc.SetRotation(meta.rotation);

        builder.With<TransformComponent>(tc);

        MeshComponent mesh;
        mesh.ModelPtr = AssetManager::GetModel(GetModelPath(type));
        builder.With<MeshComponent>(mesh);

        return builder.Build();
    }

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

        glm::ivec2 myCell = GridSystem::WorldToCell(myTransform->GetPosition());

        // --- ARCHITEKTONICZNA BLOKADA CELU (STICKY LOCK) ---
        // Je�li maszyna ma ju� namierzony talerz, najpierw sprawdzamy jego aktualn� pozycj�.
        // Zapobiega to chaotycznemu prze��czaniu cel�w, gdy inny talerz podjedzie bli�ej.
        if (m_LastHighlightedPlate.id != std::numeric_limits<std::size_t>::max())
        {
            auto* plateTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_LastHighlightedPlate);
            if (plateTransform)
            {
                glm::ivec2 plateCell = GridSystem::WorldToCell(plateTransform->GetPosition());

                // Sprawdzamy, czy ten konkretny talerz nadal znajduje si� w zasi�gu s�siednich kratek (promie� 1)
                if (std::abs(myCell.x - plateCell.x) <= 1 && std::abs(myCell.y - plateCell.y) <= 1)
                {
                    // Talerz wci�� jest w zasi�gu! Zwracamy go natychmiast i ignorujemy reszt� �wiata.
                    return m_LastHighlightedPlate;
                }
            }
        }

        // Je�li nie by�o zablokowanego talerza lub stary uciek� z zasi�gu, szukamy nowego:
        Entity closestPlate = { std::numeric_limits<std::size_t>::max(), 0 };
        float closestDist = 999.0f;

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

    virtual void OnTransferToPlate(Entity plateEntity) {}

    virtual void ResetMachineState()
    {
        m_IsReady = false;
        m_CurrentTime = 0.0f;
        m_Ingredients.clear();
        UpdateVisuals();
    }

    virtual void UpdateVisuals() = 0;

    void SetPlateHighlight(Entity plateEntity, bool state)
    {
        if (plateEntity.id == std::numeric_limits<std::size_t>::max()) return;

        if (state)
        {
            GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                    plateEntity,
                    glm::vec3(1.0f, 0.9f, 0.0f),
                    0.0f,
                    true
            });

            auto* tagSet = GetScene()->GetWorld().GetComponentVector<TagComponent>();
            if (tagSet) {
                for (size_t i = 0; i < tagSet->dense.size(); ++i) {
                    Entity childEntity = tagSet->reverse[i];
                    if (GetScene()->GetParent(childEntity).id == plateEntity.id) {
                        GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                                childEntity, glm::vec3(1.0f, 0.9f, 0.0f), 0.0f, true
                        });
                    }
                }
            }
        }
    }

    void ClearHighlight()
    {
        if (m_LastHighlightedPlate.id != std::numeric_limits<std::size_t>::max())
        {
            m_LastHighlightedPlate = { std::numeric_limits<std::size_t>::max(), 0 };
        }
    }

    void PlaceSpawnedFoodOnPlate(Entity plate)
    {
        if (m_SpawnedFood.id == std::numeric_limits<std::size_t>::max()) return;

        auto* scriptComp = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(plate);
        if (!scriptComp) return;

        PlateScript* targetPlateScript = nullptr;
        for (auto& s : scriptComp->Scripts) {
            targetPlateScript = dynamic_cast<PlateScript*>(s.Instance);
            if (targetPlateScript) break;
        }

        if (!targetPlateScript) {
            spdlog::warn("Cel nie jest talerzem lub nie ma PlateScript!");
            return;
        }

        bool success = targetPlateScript->ReceiveFinishedDish(m_SpawnedFood);

        if (success)
        {
            AudioEngine::Play("assets://sounds/plate_down.wav");

            m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
        }
        else
        {
            spdlog::info("Maszyna zatrzymuje jedzenie, bo talerz odrzucil transfer.");
        }
    }
};