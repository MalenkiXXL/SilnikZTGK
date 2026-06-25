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

    IngredientType GetPotResult() const
    {
        if (m_Ingredients.size() == 1)
        {
            if (m_Ingredients[0] == IngredientType::ChoppedTomato) return IngredientType::TomatoSoup;
            if (m_Ingredients[0] == IngredientType::RawKopytkaDough) return IngredientType::Kopytka;
        }
        else if (m_Ingredients.size() == 2)
        {
            bool hasRaspberry = (m_Ingredients[0] == IngredientType::ChoppedRaspberry || m_Ingredients[1] == IngredientType::ChoppedRaspberry);
            bool hasDust = (m_Ingredients[0] == IngredientType::SleepyDust || m_Ingredients[1] == IngredientType::SleepyDust);

            if (hasRaspberry && hasDust) return IngredientType::Candy;
        }
        return IngredientType::None;
    }

public:
    bool CanAcceptIngredient(IngredientType type) override
    {
        if (m_Ingredients.size() >= 2) return false;

        if (m_Ingredients.empty()) {
            return type == IngredientType::ChoppedTomato ||
                type == IngredientType::ChoppedRaspberry ||
                type == IngredientType::SleepyDust ||
                type == IngredientType::RawKopytkaDough;
        }

        if (m_Ingredients.size() == 1) {
            if (m_Ingredients[0] == IngredientType::ChoppedRaspberry) return type == IngredientType::SleepyDust;
            if (m_Ingredients[0] == IngredientType::SleepyDust) return type == IngredientType::ChoppedRaspberry || type == IngredientType::RawKopytkaDough;
            if (m_Ingredients[0] == IngredientType::RawKopytkaDough) return type == IngredientType::SleepyDust;
        }
        return false;
    }

    void OnCreate() override
    {
        MachineScript::OnCreate();
        m_CookTime = 3.0f;
        SetSmoking(false);
    }

    void OnDestroy() override
    {
        StopBoilingSound();
        MachineScript::OnDestroy();
    }

    void ResetMachineState() override
    {
        StopBoilingSound();
        SetSmoking(false);
        MachineScript::ResetMachineState();
    }

    void OnUpdate(Timestep ts) override
    {
        MachineScript::OnUpdate(ts);

        if (m_IsHeld) return;

        if (!m_IsReady && GetPotResult() != IngredientType::None)
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

    void OnHoverCursor() override
    {
        MachineScript::OnHoverCursor();

        if (!m_Ingredients.empty() && GetPotResult() == IngredientType::None)
        {
            GetScene()->GetWorld().GetEventBus().Publish(MachineNeedsMoreIngredientsEvent{
                    m_Entity, 0.2f
            });
        }
    }

    bool AddIngredient(IngredientType type, const std::vector<IngredientType>& pastIngredients = {}, const std::vector<std::string>& pastMachines = {}) override
    {
        if (!CanAcceptIngredient(type))
        {
            spdlog::warn("Garnek: Nie mozesz tego wrzucic!");
            return false;
        }

        m_Ingredients.push_back(type);

        m_DeepHistory.insert(m_DeepHistory.end(), pastIngredients.begin(), pastIngredients.end());
        m_DeepHistory.push_back(type);
        m_MachineHistory.insert(m_MachineHistory.end(), pastMachines.begin(), pastMachines.end());

        m_IsReady = false;

        if (GetPotResult() != IngredientType::None)
        {
            SetSmoking(true);
            if (!m_BoilingSound)
            {
                m_BoilingSound = AudioEngine::PlayLoopingSound("CookingStation/Assets/sounds/Risotto_Boil_pot.wav", 0.15f);
            }
            m_CurrentTime = 0.0f;
            spdlog::info("Garnek: Skladniki skompletowane, zaczynamy gotowanie!");
        }
        else
        {
            spdlog::info("Garnek: Przyjeto skladnik, czekam na reszte...");
            GetScene()->GetWorld().GetEventBus().Publish(MachineNeedsMoreIngredientsEvent{
                    m_Entity, 0.5f
            });
        }

        UpdateVisuals();
        AudioEngine::Play("assets://sounds/put_on_conveyor.mp3");
        return true;
    }

protected:
    void UpdateVisuals() override
    {
        if (m_IsReady)
        {
            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max())
                return;

            IngredientType resultDish = GetPotResult();

            if (resultDish == IngredientType::TomatoSoup && !GameProgress::IsRecipeUnlocked("TomatoSoup")) {
                GameProgress::UnlockRecipe("TomatoSoup");
            }
            else if (resultDish == IngredientType::Kopytka && !GameProgress::IsRecipeUnlocked("Kopytka")) {
                GameProgress::UnlockRecipe("Kopytka");
            }
            else if (resultDish == IngredientType::Candy && !GameProgress::IsRecipeUnlocked("Candy")) {
                GameProgress::UnlockRecipe("Candy");
            }

            auto* myTransform = GetComponent<TransformComponent>();
            if (!myTransform) return;

            std::string tagStr = "W_Garnku";
            if (resultDish == IngredientType::Candy) tagStr = "CukierekWGarnku";
            else if (resultDish == IngredientType::Kopytka) tagStr = "KopytkaWGarnku";
            else if (resultDish == IngredientType::GoldenKopytka) tagStr = "ZloteKopytkaWGarnku";

            m_SpawnedFood = SpawnMachineFood(resultDish, tagStr);

            auto* foodTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);
            if (foodTf)
            {
                float yOff = (resultDish == IngredientType::Candy) ? 0.7f : 1.0f;
                foodTf->SetPosition(myTransform->GetPosition() + glm::vec3(0.0f, yOff, 0.0f));
            }

            GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                    m_SpawnedFood, glm::vec3(1.0f, 0.2f, 0.6f), 1.5f, false
                });
            GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                    m_Entity, glm::vec3(1.0f, 0.2f, 0.6f), 1.5f, false
                });

            DishHistory history;
            history.BaseIngredients = m_DeepHistory;
            history.MachineHistory = m_MachineHistory;
            history.MachineHistory.push_back("Pot");
            history.OriginMachine = "Pot";
            GetScene()->GetWorld().GetEventBus().Publish(DishCreatedEvent{ m_SpawnedFood, history });
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
                SetSmoking(false);
            }
        }
    }

    void OnTransferToPlate(Entity plate) override
    {
        auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(plate);
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
            if (pScript->AddIngredient(GetPotResult(), m_IsAutomated))
            {
                if (!m_DeepHistory.empty()) {
                    pScript->m_DeepHistory.insert(pScript->m_DeepHistory.end(), m_DeepHistory.begin(), m_DeepHistory.end());
                }
                pScript->m_MachineHistory.insert(pScript->m_MachineHistory.end(), m_MachineHistory.begin(), m_MachineHistory.end());
                pScript->m_MachineHistory.push_back("Pot");

                if (!pScript->m_VisualModels.empty()) {
                    Entity newVisualOnPlate = pScript->m_VisualModels.back();

                    DishHistory history;
                    history.BaseIngredients = pScript->m_DeepHistory;
                    history.MachineHistory = pScript->m_MachineHistory;
                    history.OriginMachine = "Pot";

                    GetScene()->GetWorld().GetEventBus().Publish(DishCreatedEvent{ newVisualOnPlate, history });
                }

                spdlog::info("Garnek: Danie przelozone na talerz i zarejestrowane w pamici pod nowym ID!");

                if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max()) {
                    GetScene()->DestroyEntity(m_SpawnedFood);
                    m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
                }
                ResetMachineState();
            }
        }
    }
};