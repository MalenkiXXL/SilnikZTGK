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
            // Zabezpieczenie przed wrzuceniem dwa razy tego samego
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

    // Hybrydowe przenoszenie
    void TryTransferToPlate() override
    {
        Entity targetPlate = m_LastHighlightedPlate;

        if (targetPlate.id == std::numeric_limits<std::size_t>::max())
            targetPlate = GetClosestAvailablePlate();

        if (targetPlate.id != std::numeric_limits<std::size_t>::max() || m_IsAutomated)
        {
            // Przeniesienie na talerz
            MachineScript::TryTransferToPlate();
        }
        else
        {
            // Zabranie do reki (DragAndDrop)
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

            // Rejestracja pochodzenia
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