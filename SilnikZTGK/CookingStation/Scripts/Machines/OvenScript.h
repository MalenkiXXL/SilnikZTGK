#pragma once
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include "CookingStation/Scripts/DragAndDropScript.h"
#include "CookingStation/Core/GameProgress.h"
#include "CookingStation/Events/GameEvents.h"

class OvenScript : public MachineScript
{
public:
    void OnCreate() override
    {
        MachineScript::OnCreate();
        m_CookTime = 8.0f;
    }

    void OnUpdate(Timestep ts) override
    {
        MachineScript::OnUpdate(ts);
        if (m_IsHeld) return;

        if (!m_Ingredients.empty() && !m_IsReady)
        {
            m_CurrentTime += ts.GetSeconds();
            if (m_CurrentTime >= m_CookTime)
            {
                m_IsReady = true;
                AudioEngine::Play("assets://sounds/dish_ready.mp3");
                UpdateVisuals();
            }
        }

        if (m_IsAutomated && m_IsReady)
        {
            TryTransferToPlate();
        }
    }

    bool AddIngredient(IngredientType type) override
    {
        if (m_IsReady || !m_Ingredients.empty()) return false;

        if (type == IngredientType::RawDough)
        {
            m_Ingredients.push_back(type);
            m_IsReady = false;
            m_CurrentTime = 0.0f;
            spdlog::info("Piekarnik: Rozpoczeto pieczenie!");
            return true;
        }

        spdlog::warn("Piekarnik: Do piekarnika wrzucaj tylko wyrobione ciasto!");
        return false;
    }

    void TryTransferToPlate() override
    {
        Entity targetPlate = m_LastHighlightedPlate;

        if (targetPlate.id == std::numeric_limits<std::size_t>::max())
            targetPlate = GetClosestAvailablePlate();

        if (targetPlate.id != std::numeric_limits<std::size_t>::max())
        {
            // Przeniesienie logiczne na talerz
            auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(targetPlate);
            PlateScript* pScript = nullptr;
            if (nsc) {
                for (auto& s : nsc->Scripts) {
                    if (s.Name == "PlateScript" && s.Instance) {
                        pScript = static_cast<PlateScript*>(s.Instance);
                        break;
                    }
                }
            }

            if (pScript)
            {
                if (pScript->AddIngredient(IngredientType::Baguette))
                {
                    spdlog::info("Piekarnik: Bagietka gotowa i przelozona na talerz!");
                    ClearHighlight();
                    if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max()) {
                        GetScene()->DestroyEntity(m_SpawnedFood);
                        m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
                    }
                    ResetMachineState();
                }
            }
        }
        else if (!m_IsAutomated)
        {
            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max())
            {
                GetScene()->DestroyEntity(m_SpawnedFood);
                m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };

                DragAndDropScript::StartDrag(IngredientType::Baguette, "assets://models/skladniki/bagietka/bagietka.gltf");
                ResetMachineState();
                ClearHighlight();
            }
        }
    }

protected:
    void UpdateVisuals() override
    {
        if (m_IsReady)
        {
            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max()) return;

            if (!GameProgress::IsRecipeUnlocked("Baguette"))
            {
                GameProgress::UnlockRecipe("Baguette");
                spdlog::info("Piekarnik: Przepis na bagietke odblokowany!");
            }

            auto* myTransform = GetComponent<TransformComponent>();
            if (!myTransform) return;

            m_SpawnedFood = SpawnMachineFood(IngredientType::Baguette, "assets://models/skladniki/bagietka/bagietka.gltf", "BagietkaWPiekarniku");

            auto* foodTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);
            if (foodTf)
            {
                foodTf->SetPosition(myTransform->GetPosition() + glm::vec3(0.0f, 1.0f, 0.0f));
            }

            GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                    m_SpawnedFood, glm::vec3(1.0f, 0.2f, 0.6f), 1.5f, false
            });
            GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                    m_Entity, glm::vec3(1.0f, 0.2f, 0.6f), 1.5f, false
            });

            DishHistory history;
            history.BaseIngredients = m_Ingredients;
            history.OriginMachine = "Oven";
            GetScene()->GetWorld().GetEventBus().Publish(DishCreatedEvent{ m_SpawnedFood, history });

            spdlog::info("Piekarnik: Bagietka gotowa!");
        }
        else
        {
            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max())
            {
                GetScene()->DestroyEntity(m_SpawnedFood);
                m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
            }
        }
    }

    void OnTransferToPlate(Entity plate) override
    {
        PlaceSpawnedFoodOnPlate(plate);
    }
};