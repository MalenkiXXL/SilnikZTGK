#pragma once
#include "MachineScript.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h" 
#include "CookingStation/Core/GameProgress.h"
#include "CookingStation/Scripts/ParticleEmitterScript.h"

class PotScript : public MachineScript
{
private:
    void SetSmoking(bool state, bool clearInstatly = false)
    {
        auto* scriptComp = GetComponent<NativeScriptComponent>();
        if (scriptComp)
        {
            for (auto& s : scriptComp->Scripts)
            {
                if (s.Name == "SteamEmitterScript" && s.Instance)
                {
                    auto* emitter = static_cast<ParticleEmitterScript*>(s.Instance);
                    if (state)
                        emitter->Play();
                    else
                        emitter->Stop();
                    if (clearInstatly)
                        emitter->Clear();
                    break;
                }
            }
        }
    }

public:
    void OnCreate() override
    {
        MachineScript::OnCreate();
        m_CookTime = 3.0f;
        SetSmoking(false);
    }

    void OnUpdate(Timestep ts) override
    {
        MachineScript::OnUpdate(ts);

        if (m_IsHeld) return;

        // Logika gotowania
        if (!m_Ingredients.empty() && !m_IsReady)
        {
            m_CurrentTime += ts.GetSeconds();

            if (m_CurrentTime >= m_CookTime)
            {
                m_IsReady = true;
                UpdateVisuals();
            }
            if (m_CookTime - m_CurrentTime <= 0.8f)
            {
                SetSmoking(false);
            }
        }

        // Automatyzacja 
        if (m_IsAutomated && m_IsReady)
        {
            TryTransferToPlate();
        }
    }

    virtual void HandleClick() override
    {
        MachineScript::HandleClick();
    }

    bool AddIngredient(IngredientType type) override
    {
        if (m_IsReady || m_Ingredients.size() >= 2) return false;

        if (type == IngredientType::ChoppedTomato)
        {
            m_Ingredients.push_back(type);
            m_IsReady = false;
            SetSmoking(true);
            m_CurrentTime = 0.0f;
            UpdateVisuals();
            spdlog::info("Garnek: Przyjeto pokrojonego pomidora, zaczynamy gotowanie!");
            return true;
        }

        spdlog::warn("Garnek: Nie wrzucaj tego! Najpierw pokroj na desce!");
        return false;
    }

protected:
    void UpdateVisuals() override
    {
        if (m_IsReady)
        {
            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max())
                return;

            if (!GameProgress::IsRecipeUnlocked("TomatoSoup"))
            {
                GameProgress::UnlockRecipe("TomatoSoup");
                spdlog::info("Zupa pomidorowa odblokowana po raz pierwszy!");
            }

            auto* myTransform = GetComponent<TransformComponent>();
            if (!myTransform) return;

            m_SpawnedFood = SpawnMachineFood(IngredientType::None, "CookingStation/Assets/models/skladniki/pomidor/pomidorowa.gltf", "W_Garnku");

            // Zupa  zawsze wyl¹duje dok³adnie o 1.0 wy¿ej od garnka
            auto* foodTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);
            if (foodTf)
            {
                foodTf->SetPosition(myTransform->GetPosition() + glm::vec3(0.0f, 1.0f, 0.0f));
            }

            spdlog::info("Danie gotowe, pojawia sie nad garnkiem.");
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