#pragma once
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include "CookingStation/Scripts/DragAndDropScript.h"
#include "CookingStation/Core/GameProgress.h"
#include "CookingStation/Events/GameEvents.h"

class OvenScript : public MachineScript
{
public:
    IngredientType GetBakedType() const
    {
        if (m_Ingredients.empty()) return IngredientType::None;
        if (m_Ingredients[0] == IngredientType::RawApplePie) return IngredientType::ApplePie;
        return IngredientType::Baguette;
    }

    bool CanAcceptIngredient(IngredientType type) override
    {
        if (m_IsReady || !m_Ingredients.empty()) return false;
        return type == IngredientType::RawDough || type == IngredientType::RawApplePie;
    }

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
        if (CanAcceptIngredient(type))
        {
            m_Ingredients.push_back(type);
            m_IsReady = false;
            m_CurrentTime = 0.0f;
            spdlog::info("Piekarnik: Rozpoczeto pieczenie!");

            auto* meshComp = GetComponent<MeshComponent>();
            if (meshComp)
            {
                meshComp->Path = "assets://models/przybory_kuchenne/piekarnik/piekarnik_on.gltf";
                meshComp->ModelPtr = AssetManager::GetModel(meshComp->Path);
            }

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
                IngredientType bakedType = GetBakedType();
                if (pScript->AddIngredient(bakedType))
                {
                    spdlog::info("Piekarnik: Wypiek gotowy i przelozony na talerz!");
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

                IngredientType bakedType = GetBakedType();
                DragAndDropScript::StartDrag(bakedType);
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

            IngredientType bakedType = GetBakedType();

            if (bakedType == IngredientType::Baguette && !GameProgress::IsRecipeUnlocked("Baguette"))
            {
                GameProgress::UnlockRecipe("Baguette");
                spdlog::info("Piekarnik: Przepis na bagietke odblokowany!");
            }
            else if (bakedType == IngredientType::ApplePie && !GameProgress::IsRecipeUnlocked("ApplePie"))
            {
                GameProgress::UnlockRecipe("ApplePie");
                spdlog::info("Piekarnik: Przepis na szarlotke odblokowany!");
            }

            auto* meshComp = GetComponent<MeshComponent>();
            if (meshComp)
            {
                meshComp->Path = "assets://models/przybory_kuchenne/piekarnik/piekarnik.gltf";
                meshComp->ModelPtr = AssetManager::GetModel(meshComp->Path);
            }

            auto* myTransform = GetComponent<TransformComponent>();
            if (!myTransform) return;

            std::string tagStr = (bakedType == IngredientType::ApplePie) ? "SzarlotkaWPiekarniku" : "BagietkaWPiekarniku";
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
            history.BaseIngredients = m_Ingredients;
            history.OriginMachine = "Oven";
            GetScene()->GetWorld().GetEventBus().Publish(DishCreatedEvent{ m_SpawnedFood, history });

            spdlog::info("Piekarnik: Wypiek gotowy!");
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
    void OnTransferToPlate(Entity plate) override
    {
        PlaceSpawnedFoodOnPlate(plate);
    }
};