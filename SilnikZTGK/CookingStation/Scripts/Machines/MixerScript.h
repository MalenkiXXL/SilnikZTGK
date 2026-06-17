#pragma once
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include "CookingStation/Scripts/DragAndDropScript.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Events/GameEvents.h"

class MixerScript : public MachineScript
{
public:
    void OnCreate() override
    {
        MachineScript::OnCreate();
        m_CookTime = 4.0f;
    }

    void OnUpdate(Timestep ts) override
    {
        MachineScript::OnUpdate(ts);
        if (m_IsHeld) return;

        // Logika mieszania
        if (!m_IsReady)
        {
            bool hasFlour = std::find(m_Ingredients.begin(), m_Ingredients.end(), IngredientType::Flour) != m_Ingredients.end();
            bool hasMilk = std::find(m_Ingredients.begin(), m_Ingredients.end(), IngredientType::Milk) != m_Ingredients.end();

            if (hasFlour && hasMilk)
            {
                m_CurrentTime += ts.GetSeconds();
                if (m_CurrentTime >= m_CookTime)
                {
                    m_IsReady = true;
                    AudioEngine::Play("assets://sounds/dish_ready.mp3");
                    UpdateVisuals();
                }
            }
        }

        // Automatyzacja 
        if (m_IsAutomated && m_IsReady)
        {
            TryTransferToPlate();
        }
    }

    bool AddIngredient(IngredientType type) override
    {
        if (m_IsReady || m_Ingredients.size() >= 2) return false;

        if (type == IngredientType::Flour || type == IngredientType::Milk)
        {
            if (std::find(m_Ingredients.begin(), m_Ingredients.end(), type) != m_Ingredients.end())
            {
                spdlog::warn("Mikser: Ten skladnik juz tu jest!");
                return false;
            }

            m_Ingredients.push_back(type);
            m_IsReady = false;
            m_CurrentTime = 0.0f;
            spdlog::info("Mikser: Przyjeto skladnik!");
            return true;
        }

        return false;
    }

    void TryTransferToPlate() override
    {
        Entity targetPlate = m_LastHighlightedPlate;

        if (targetPlate.id == std::numeric_limits<std::size_t>::max())
            targetPlate = GetClosestAvailablePlate();

        if (targetPlate.id != std::numeric_limits<std::size_t>::max())
        {
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
                if (pScript->AddIngredient(IngredientType::RawDough))
                {
                    spdlog::info("Mikser: Ciasto logicznie przeniesione na talerz!");
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

                DragAndDropScript::StartDrag(IngredientType::RawDough, "assets://models/skladniki/maka/maka.gltf");
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

            auto* myTransform = GetComponent<TransformComponent>();
            if (!myTransform) return;

            m_SpawnedFood = SpawnMachineFood(IngredientType::RawDough, "assets://models/skladniki/maka/maka.gltf", "WyrobioneCiasto");

            auto* foodTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);
            if (foodTf)
            {
                foodTf->SetPosition(myTransform->GetPosition() + glm::vec3(0.0f, 1.2f, 0.0f));
            }

            GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                    m_SpawnedFood, glm::vec3(1.0f, 0.2f, 0.6f), 1.5f, false
            });
            GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                    m_Entity, glm::vec3(1.0f, 0.2f, 0.6f), 1.5f, false
            });

            DishHistory history;
            history.BaseIngredients = m_Ingredients;
            history.OriginMachine = "Mixer";
            GetScene()->GetWorld().GetEventBus().Publish(DishCreatedEvent{ m_SpawnedFood, history });

            spdlog::info("Mikser: Ciasto gotowe!");
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