#pragma once
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include "CookingStation/Scripts/DragAndDropScript.h"
#include "CookingStation/Core/GameProgress.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Events/GameEvents.h"

class OvenScript : public MachineScript
{
public:
    ma_sound* m_BakingSoundPtr = nullptr;

    void StopBakingSound()
    {
        if (m_BakingSoundPtr)
        {
            AudioEngine::StopLoopingSound(m_BakingSoundPtr);
            m_BakingSoundPtr = nullptr;
        }
    }

    IngredientType GetBakedType() const
    {
        if (m_Ingredients.empty()) return IngredientType::None;
        if (m_Ingredients[0] == IngredientType::RawApplePie) return IngredientType::ApplePie;
        if (m_Ingredients[0] == IngredientType::RawSleepyDough) return IngredientType::SleepyBread;
        if (m_Ingredients[0] == IngredientType::RawCupcakeDough) return IngredientType::Cupcake;
        return IngredientType::Baguette;
    }

    bool CanAcceptIngredient(IngredientType type) override
    {
        if (m_IsReady || !m_Ingredients.empty()) return false;
        return type == IngredientType::RawDough || type == IngredientType::RawApplePie ||
            type == IngredientType::RawSleepyDough || type == IngredientType::RawCupcakeDough;
    }

    void OnCreate() override
    {
        MachineScript::OnCreate();
        m_CookTime = 8.0f;
    }

    void OnDestroy() override
    {
        StopBakingSound();
        MachineScript::OnDestroy();
    }

    void ResetMachineState() override
    {
        StopBakingSound();
        MachineScript::ResetMachineState();
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
                StopBakingSound();
                AudioEngine::Play("assets://sounds/dish_ready.mp3");
                UpdateVisuals();
            }
        }

        if (m_IsAutomated && m_IsReady)
        {
            TryTransferToPlate();
        }
    }

    bool AddIngredient(IngredientType type, const std::vector<IngredientType>& pastIngredients = {}, const std::vector<std::string>& pastMachines = {}) override
    {
        if (CanAcceptIngredient(type))
        {
            m_Ingredients.push_back(type);

            // Łączenie historii w całość
            m_DeepHistory.insert(m_DeepHistory.end(), pastIngredients.begin(), pastIngredients.end());
            m_DeepHistory.push_back(type);
            m_MachineHistory.insert(m_MachineHistory.end(), pastMachines.begin(), pastMachines.end());

            m_IsReady = false;
            m_CurrentTime = 0.0f;
            spdlog::info("Piekarnik: Rozpoczeto pieczenie!");

            if (!m_BakingSoundPtr) {
                m_BakingSoundPtr = AudioEngine::PlayLoopingSound("assets://sounds/baking.mp3", 0.15f);
            }

            auto* meshComp = GetComponent<MeshComponent>();
            if (meshComp)
            {
                meshComp->Path = "assets://models/przybory_kuchenne/piekarnik/piekarnik_on.gltf";
                meshComp->ModelPtr = AssetManager::GetModel(meshComp->Path);
            }

            AudioEngine::Play("assets://sounds/put_on_conveyor.mp3");
            return true;
        }

        spdlog::warn("Piekarnik: Do piekarnika wrzucaj tylko wyrobione ciasto!");
        AudioEngine::Play("assets://sounds/error.mp3");
        return false;
    }

    void TryTransferToPlate() override
    {
        Entity targetPlate = GetClosestAvailablePlate();

        if (targetPlate.id != std::numeric_limits<std::size_t>::max())
        {
            ClearHighlight();
            Entity foodBeforeTransfer = m_SpawnedFood;

            PlaceSpawnedFoodOnPlate(targetPlate);

            if (m_SpawnedFood.id != foodBeforeTransfer.id)
            {
                ResetMachineState();
            }
        }
        else if (!m_IsAutomated)
        {
            spdlog::warn("Piekarnik: Brak talerza w zasiegu!");
            AudioEngine::Play("assets://sounds/error.mp3");
        }
    }

protected:
    void UpdateVisuals() override
    {
        if (m_IsReady)
        {
            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max()) return;

            IngredientType bakedType = GetBakedType();

            if (bakedType == IngredientType::Baguette && !GameProgress::IsRecipeUnlocked("Baguette")) {
                GameProgress::UnlockRecipe("Baguette");
                spdlog::info("Piekarnik: Przepis na bagietke odblokowany!");
            }
            else if (bakedType == IngredientType::ApplePie && !GameProgress::IsRecipeUnlocked("ApplePie")) {
                GameProgress::UnlockRecipe("ApplePie");
                spdlog::info("Piekarnik: Przepis na szarlotke odblokowany!");
            }
            else if (bakedType == IngredientType::SleepyBread && !GameProgress::IsRecipeUnlocked("SleepyBread")) {
                GameProgress::UnlockRecipe("SleepyBread");
                spdlog::info("Piekarnik: Przepis na spiacy chleb odblokowany!");
            }
            else if (bakedType == IngredientType::Cupcake && !GameProgress::IsRecipeUnlocked("Cupcake")) {
                GameProgress::UnlockRecipe("Cupcake");
                spdlog::info("Piekarnik: Przepis na babeczke odblokowany!");
            }

            auto* meshComp = GetComponent<MeshComponent>();
            if (meshComp)
            {
                meshComp->Path = "assets://models/przybory_kuchenne/piekarnik/piekarnik.gltf";
                meshComp->ModelPtr = AssetManager::GetModel(meshComp->Path);
            }

            auto* myTransform = GetComponent<TransformComponent>();
            if (!myTransform) return;

            std::string tagStr = "BagietkaWPiekarniku";
            if (bakedType == IngredientType::ApplePie) tagStr = "SzarlotkaWPiekarniku";
            else if (bakedType == IngredientType::SleepyBread) tagStr = "SpiacyChlebWPiekarniku";
            else if (bakedType == IngredientType::Cupcake) tagStr = "BabeczkaWPiekarniku";

            m_SpawnedFood = SpawnMachineFood(bakedType, tagStr);

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
            history.BaseIngredients = m_DeepHistory;
            history.MachineHistory = m_MachineHistory; // <-- Nowość
            history.MachineHistory.push_back("Oven");  // <-- Nowość
            history.OriginMachine = "Oven";
            GetScene()->GetWorld().GetEventBus().Publish(DishCreatedEvent{ m_SpawnedFood, history });
        }
        else
        {
            auto* meshComp = GetComponent<MeshComponent>();
            if (meshComp)
            {
                meshComp->Path = "assets://models/przybory_kuchenne/piekarnik/piekarnik.gltf";
                meshComp->ModelPtr = AssetManager::GetModel(meshComp->Path);
            }

            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max())
            {
                GetScene()->DestroyEntity(m_SpawnedFood);
                m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
            }
        }
    }

    void OnTransferToPlate(Entity plate) override {}
};