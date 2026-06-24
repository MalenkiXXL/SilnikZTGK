#pragma once
#include "MachineScript.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Core/GameProgress.h"
#include "CookingStation/Scripts/ParticleEmitterScript.h"

class PotScript : public MachineScript
{
private:
    ma_sound* m_BoilingSound = nullptr;

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

    void StopBoilingSound()
    {
        if (m_BoilingSound)
        {
            AudioEngine::StopLoopingSound(m_BoilingSound);
            m_BoilingSound = nullptr;
        }
    }

public:
    void OnCreate() override
    {
        MachineScript::OnCreate();
        m_CookTime = 3.0f;
        SetSmoking(false);
    }

    void OnDestroy() override
    {
        StopBoilingSound();
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
                AudioEngine::Play("CookingStation/Assets/sounds/dish_ready.mp3");
                UpdateVisuals();
            }
            if (m_CookTime - m_CurrentTime <= 0.8f)
            {
                SetSmoking(false);
                StopBoilingSound();
            }
        }

        if (m_IsAutomated && m_IsReady)
        {
            TryTransferToPlate();
        }
    }

    virtual void HandleClick() override
    {
        MachineScript::HandleClick();
    }

    bool AddIngredient(IngredientType type, const std::vector<IngredientType>& pastIngredients = {}, const std::vector<std::string>& pastMachines = {}) override
    {
        if (m_IsReady || m_Ingredients.size() >= 2) return false;

        if (type == IngredientType::ChoppedTomato)
        {
            m_Ingredients.push_back(type);

            m_DeepHistory.insert(m_DeepHistory.end(), pastIngredients.begin(), pastIngredients.end());
            m_DeepHistory.push_back(type);
            m_MachineHistory.insert(m_MachineHistory.end(), pastMachines.begin(), pastMachines.end());

            m_IsReady = false;

            SetSmoking(true);

            if (!m_BoilingSound) {
                m_BoilingSound = AudioEngine::PlayLoopingSound("CookingStation/Assets/sounds/Risotto_Boil_pot.wav", 0.15f);
            }

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

            m_SpawnedFood = SpawnMachineFood(IngredientType::TomatoSoup, "W_Garnku");

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
            history.MachineHistory.push_back("Pot");   // <-- Nowość
            history.OriginMachine = "Pot";
            GetScene()->GetWorld().GetEventBus().Publish(DishCreatedEvent{ m_SpawnedFood, history });

            spdlog::info("Danie gotowe, pojawia sie nad garnkiem i zostalo wpisane do rejestru.");
        }
        else
        {
            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max())
            {
                GetScene()->DestroyEntity(m_SpawnedFood);
                m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
            }

            if (m_Ingredients.empty())
            {
                StopBoilingSound();
            }
        }
    }

    void OnTransferToPlate(Entity plate) override
    {
        PlaceSpawnedFoodOnPlate(plate);
    }
};