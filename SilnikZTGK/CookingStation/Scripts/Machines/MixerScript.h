#pragma once
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include "CookingStation/Scripts/DragAndDropScript.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Core/AudioEngine.h"

class MixerScript : public MachineScript
{
private:
    IngredientType GetMixerResult() const
    {
        bool hasMilk = std::find(m_Ingredients.begin(), m_Ingredients.end(), IngredientType::Milk) != m_Ingredients.end();
        bool hasFlour = std::find(m_Ingredients.begin(), m_Ingredients.end(), IngredientType::Flour) != m_Ingredients.end();
        bool hasChoppedApple = std::find(m_Ingredients.begin(), m_Ingredients.end(), IngredientType::ChoppedApple) != m_Ingredients.end();
        bool hasSleepyDust = std::find(m_Ingredients.begin(), m_Ingredients.end(), IngredientType::SleepyDust) != m_Ingredients.end();
        bool hasChoppedRaspberry = std::find(m_Ingredients.begin(), m_Ingredients.end(), IngredientType::ChoppedRaspberry) != m_Ingredients.end();
        bool hasChoppedPotato = std::find(m_Ingredients.begin(), m_Ingredients.end(), IngredientType::ChoppedPotato) != m_Ingredients.end();
        bool hasYawn = std::find(m_Ingredients.begin(), m_Ingredients.end(), IngredientType::Yawn) != m_Ingredients.end();

        if (m_Ingredients.size() == 3) {
            if (hasMilk && hasFlour && hasChoppedApple) return IngredientType::RawApplePie;
            if (hasMilk && hasFlour && hasSleepyDust) return IngredientType::RawSleepyDough;
            if (hasMilk && hasFlour && hasChoppedRaspberry) return IngredientType::RawCupcakeDough;
            if (hasMilk && hasFlour && hasYawn) return IngredientType::RawSleepyDough;

            return IngredientType::None;
        }

        if (m_Ingredients.size() == 2) {
            if (hasFlour && hasChoppedPotato) return IngredientType::RawKopytkaDough;

            if (!hasMilk) return IngredientType::None;
            if (hasFlour) return IngredientType::RawDough;

            if (std::find(m_Ingredients.begin(), m_Ingredients.end(), IngredientType::Apple) != m_Ingredients.end())
                return IngredientType::AppleShake;
            if (std::find(m_Ingredients.begin(), m_Ingredients.end(), IngredientType::Raspberry) != m_Ingredients.end())
                return IngredientType::RaspberryShake;
            if (std::find(m_Ingredients.begin(), m_Ingredients.end(), IngredientType::Strawberry) != m_Ingredients.end())
                return IngredientType::StrawberryShake;
            if (std::find(m_Ingredients.begin(), m_Ingredients.end(), IngredientType::CoffeeBeans) != m_Ingredients.end())
                return IngredientType::CoffeeShake;
        }

        return IngredientType::None;
    }

public:
    bool CanAcceptIngredient(IngredientType type) override
    {
        bool incomingIsRaw = IsRaw(type);
        bool incomingIsChopped = IsChopped(type);

        for (auto existing : m_Ingredients) {
            if (incomingIsRaw && IsChopped(existing)) return false;
            if (incomingIsChopped && IsRaw(existing)) return false;
        }

        if (m_IsReady && GetMixerResult() == IngredientType::RawDough && incomingIsChopped) {
            return false;
        }

        if (m_IsReady) {
            if (GetMixerResult() == IngredientType::RawDough &&
                (type == IngredientType::ChoppedApple || type == IngredientType::SleepyDust || type == IngredientType::ChoppedRaspberry || type == IngredientType::Yawn)) return true;
            return false;
        }

        if (m_Ingredients.size() >= 3) return false;

        if (std::find(m_Ingredients.begin(), m_Ingredients.end(), type) != m_Ingredients.end())
            return false;

        std::vector<IngredientType> testList = m_Ingredients;
        testList.push_back(type);

        bool hasMilk = std::find(testList.begin(), testList.end(), IngredientType::Milk) != testList.end();
        bool hasFlour = std::find(testList.begin(), testList.end(), IngredientType::Flour) != testList.end();
        bool hasApple = std::find(testList.begin(), testList.end(), IngredientType::ChoppedApple) != testList.end();
        bool hasDust = std::find(testList.begin(), testList.end(), IngredientType::SleepyDust) != testList.end();
        bool hasRaspberry = std::find(testList.begin(), testList.end(), IngredientType::ChoppedRaspberry) != testList.end();
        bool hasPotato = std::find(testList.begin(), testList.end(), IngredientType::ChoppedPotato) != testList.end();
        bool hasYawn = std::find(testList.begin(), testList.end(), IngredientType::Yawn) != testList.end();

        if (testList.size() == 3) {
            return (hasMilk && hasFlour && (hasApple || hasDust || hasRaspberry || hasYawn));
        }

        if (testList.size() == 2) {
            if (hasFlour && hasPotato) return true;

            if (hasMilk) return true;
            if (hasFlour && (hasApple || hasDust || hasRaspberry || hasYawn)) return true;
            return false;
        }

        if (testList.size() == 1) {
            return type == IngredientType::Flour || type == IngredientType::Milk ||
                type == IngredientType::ChoppedApple || type == IngredientType::Apple ||
                type == IngredientType::Raspberry || type == IngredientType::Strawberry ||
                type == IngredientType::CoffeeBeans || type == IngredientType::SleepyDust ||
                type == IngredientType::ChoppedRaspberry || type == IngredientType::ChoppedPotato ||
                type == IngredientType::Yawn;
        }

        return false;
    }

    void OnCreate() override
    {
        MachineScript::OnCreate();
        m_CookTime = 4.0f;
    }

    void OnDestroy() override
    {
    }

    void OnUpdate(Timestep ts) override
    {
        MachineScript::OnUpdate(ts);
        if (m_IsHeld) return;

        if (m_IsReady && !m_IsAutomated && !GlobalIsMachineHeld)
        {
            Entity closestPlate = GetClosestAvailablePlate();
            if (closestPlate.id != std::numeric_limits<std::size_t>::max())
            {
                SetPlateHighlight(closestPlate, true);
            }
        }

        // Logika mieszania
        if (!m_IsReady)
        {
            if (GetMixerResult() != IngredientType::None)
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

        if (m_IsAutomated && m_IsReady)
        {
            TryTransferToPlate();
        }
    }

    bool AddIngredient(IngredientType type, const std::vector<IngredientType>& pastIngredients = {}, const std::vector<std::string>& pastMachines = {}) override
    {
        if (!CanAcceptIngredient(type)) return false;

        m_Ingredients.push_back(type);

        // Łączenie historii w całość
        m_DeepHistory.insert(m_DeepHistory.end(), pastIngredients.begin(), pastIngredients.end());
        m_DeepHistory.push_back(type);
        m_MachineHistory.insert(m_MachineHistory.end(), pastMachines.begin(), pastMachines.end());

        m_IsReady = false;
        m_CurrentTime = 0.0f;
        spdlog::info("Mikser: Przyjeto skladnik!");

        if (GetMixerResult() != IngredientType::None)
        {
            auto* meshComp = GetComponent<MeshComponent>();
            if (meshComp)
            {
                meshComp->Path = "assets://models/przybory_kuchenne/mikser/blender_on.gltf";
                meshComp->ModelPtr = AssetManager::GetModel(meshComp->Path);
            }

            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max()) {
                GetScene()->DestroyEntity(m_SpawnedFood);
                m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
            }

            spdlog::info("Mikser: Rozpoczynam mieszanie (zmiana modelu i dzwiek)!");
        }
        else
        {
            GetScene()->GetWorld().GetEventBus().Publish(MachineNeedsMoreIngredientsEvent{
                    m_Entity, 0.5f
                });
        }

        return true;
    }

    void OnHoverCursor() override
    {
        if (!m_IsReady && !m_Ingredients.empty() && GetMixerResult() == IngredientType::None)
        {
            GetScene()->GetWorld().GetEventBus().Publish(MachineNeedsMoreIngredientsEvent{
                    m_Entity, 0.2f
                });
        }
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
                if (pScript->AddIngredient(GetMixerResult()))
                {
                    if (!m_DeepHistory.empty()) {
                        pScript->m_DeepHistory.insert(pScript->m_DeepHistory.end(), m_DeepHistory.begin(), m_DeepHistory.end());
                    }
                    pScript->m_MachineHistory.insert(pScript->m_MachineHistory.end(), m_MachineHistory.begin(), m_MachineHistory.end());
                    pScript->m_MachineHistory.push_back("Mixer");

                    if (!pScript->m_VisualModels.empty()) {
                        Entity newVisualOnPlate = pScript->m_VisualModels.back(); 

                        DishHistory history;
                        history.BaseIngredients = pScript->m_DeepHistory;
                        history.MachineHistory = pScript->m_MachineHistory;
                        history.OriginMachine = "Mixer";

                        GetScene()->GetWorld().GetEventBus().Publish(DishCreatedEvent{ newVisualOnPlate, history });
                        spdlog::info("Mikser: Zarejestrowano poprawnie jedzenie ID: {} w GameManagerze", newVisualOnPlate.id);
                    }
                    ClearHighlight();
                    if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max()) {
                        GetScene()->DestroyEntity(m_SpawnedFood);
                        m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
                    }
                    ResetMachineState();
                }
            }
        }
        else
        {
            spdlog::warn("Mikser: Brak talerza w pobliżu! Nie można wyjąć produktu.");
            AudioEngine::Play("assets://sounds/error.mp3");
        }
    }


protected:
    void UpdateVisuals() override
    {
        if (m_IsReady)
        {
            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max()) return;

            IngredientType resultDish = GetMixerResult();

            if ((resultDish == IngredientType::AppleShake ||
                resultDish == IngredientType::RaspberryShake ||
                resultDish == IngredientType::StrawberryShake ||
                resultDish == IngredientType::CoffeeShake) && !GameProgress::IsRecipeUnlocked("Shake"))
            {
                GameProgress::UnlockRecipe("Shake");
                spdlog::info("Mikser: Przepis na Shake'a odblokowany!");
            }

            auto* meshComp = GetComponent<MeshComponent>();
            if (meshComp)
            {
                meshComp->Path = "assets://models/przybory_kuchenne/mikser/blender.gltf";
                meshComp->ModelPtr = AssetManager::GetModel(meshComp->Path);
            }

            auto* myTransform = GetComponent<TransformComponent>();
            if (!myTransform) return;

            m_SpawnedFood = SpawnMachineFood(resultDish, "GotowyProduktZMiksera");

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

            if (resultDish == IngredientType::StrawberryShake) {
                history.BaseIngredients = { IngredientType::Strawberry, IngredientType::Milk };
            }
            else if (resultDish == IngredientType::RaspberryShake) {
                history.BaseIngredients = { IngredientType::Raspberry, IngredientType::Milk };
            }
            else if (resultDish == IngredientType::CoffeeShake) {
                history.BaseIngredients = { IngredientType::CoffeeBeans, IngredientType::Milk };
            }
            else if (resultDish == IngredientType::AppleShake) {
                history.BaseIngredients = { IngredientType::Apple, IngredientType::Milk };
            }
            else {
                // Domyślne zachowanie dla reszty 
                history.BaseIngredients = m_DeepHistory;
                for (auto ing : m_Ingredients) {
                    history.BaseIngredients.push_back(ing);
                }
            }

            history.MachineHistory = m_MachineHistory;
            history.MachineHistory.push_back("Mixer");
            history.OriginMachine = "Mixer";

            GetScene()->GetWorld().GetEventBus().Publish(DishCreatedEvent{ m_SpawnedFood, history });

            spdlog::info("Mikser: Gotowe!");
        }
        else
        {
            auto* meshComp = GetComponent<MeshComponent>();
            if (meshComp)
            {
                meshComp->Path = "assets://models/przybory_kuchenne/mikser/blender.gltf";
                meshComp->ModelPtr = AssetManager::GetModel(meshComp->Path);
            }

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