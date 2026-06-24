#pragma once
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Core/AudioEngine.h"

class CoffeeMakerScript : public MachineScript
{
private:
    ma_sound* m_BrewingSound = nullptr;

    void StopBrewingSound()
    {
        if (m_BrewingSound)
        {
            AudioEngine::StopLoopingSound(m_BrewingSound);
            m_BrewingSound = nullptr;
        }
    }

public:
    bool CanAcceptIngredient(IngredientType type) override
    {
        if (m_IsReady) return false;
        if (!m_Ingredients.empty()) return false; // ju¿ coœ jest w œrodku
        return type == IngredientType::CoffeeBeans;
    }

    void OnCreate() override
    {
        MachineScript::OnCreate();
        m_CookTime = 5.0f; // czas parzenia
    }

    void OnDestroy() override
    {
        StopBrewingSound();
    }

    void OnUpdate(Timestep ts) override
    {
        MachineScript::OnUpdate(ts);
        if (m_IsHeld) return;

        if (!m_IsReady && !m_Ingredients.empty())
        {
            m_CurrentTime += ts.GetSeconds();
            if (m_CurrentTime >= m_CookTime)
            {
                m_IsReady = true;
                StopBrewingSound();
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
        if (!CanAcceptIngredient(type)) return false;

        m_Ingredients.push_back(type);
        m_IsReady = false;
        m_CurrentTime = 0.0f;
        spdlog::info("CoffeeMaker: Przyjeto ziarno kawy, rozpoczynam parzenie!");

        // zmiana modelu na "pracuj¹cy" ekspres – podmieñ œcie¿kê gdy bêdziesz mia³ model
        auto* meshComp = GetComponent<MeshComponent>();
        if (meshComp)
        {
             meshComp->Path = "assets://models/przybory_kuchenne/ekspres/ekspres.gltf";
             meshComp->ModelPtr = AssetManager::GetModel(meshComp->Path);
        }

        if (!m_BrewingSound)
        {
            m_BrewingSound = AudioEngine::PlayLoopingSound("CookingStation/Assets/sounds/mixer.mp3", 0.1f);
            // docelowo podmieñ na dŸwiêk ekspresu
        }

        return true;
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

            if (pScript && pScript->AddIngredient(IngredientType::Coffee))
            {
                spdlog::info("CoffeeMaker: Kawa przeniesiona na talerz/kubek!");
                ClearHighlight();
                if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max()) {
                    GetScene()->DestroyEntity(m_SpawnedFood);
                    m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
                }
                ResetMachineState();
            }
        }
        else if (!m_IsAutomated)
        {
            spdlog::warn("CoffeeMaker: Brakuje talerza/kubka!");
            AudioEngine::Play("assets://sounds/error.mp3");
        }
    }

protected:
    void UpdateVisuals() override
    {
        if (m_IsReady)
        {
            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max()) return;

            // powrót do modelu spoczynkowego
            auto* meshComp = GetComponent<MeshComponent>();
            if (meshComp)
            {
                 meshComp->Path = "assets://models/przybory_kuchenne/ekspres/ekspres.gltf";
                 meshComp->ModelPtr = AssetManager::GetModel(meshComp->Path);
            }

            auto* myTransform = GetComponent<TransformComponent>();
            if (!myTransform) return;

            m_SpawnedFood = SpawnMachineFood(IngredientType::Coffee, "GotowaCzKawy");

            auto* foodTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);
            if (foodTf)
                foodTf->SetPosition(myTransform->GetPosition() + glm::vec3(0.0f, 2.0f, 0.0f));

            GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                m_SpawnedFood, glm::vec3(0.6f, 0.3f, 0.1f), 1.5f, false
                });
            GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                m_Entity, glm::vec3(0.6f, 0.3f, 0.1f), 1.5f, false
                });

            DishHistory history;
            history.BaseIngredients = m_Ingredients;
            history.OriginMachine = "CoffeeMaker";
            GetScene()->GetWorld().GetEventBus().Publish(DishCreatedEvent{ m_SpawnedFood, history });

            spdlog::info("CoffeeMaker: Kawa gotowa!");
        }
        else
        {
            auto* meshComp = GetComponent<MeshComponent>();
            if (meshComp)
            {
                 meshComp->Path = "assets://models/przybory_kuchenne/ekspres/ekspres.gltf";
                 meshComp->ModelPtr = AssetManager::GetModel(meshComp->Path);
            }

            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max())
            {
                GetScene()->DestroyEntity(m_SpawnedFood);
                m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
            }

            StopBrewingSound();
        }
    }

    void OnTransferToPlate(Entity plate) override
    {
        PlaceSpawnedFoodOnPlate(plate);
    }
};