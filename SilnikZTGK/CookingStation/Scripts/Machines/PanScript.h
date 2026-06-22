#pragma once
#include "MachineScript.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Core/GameProgress.h"
#include "CookingStation/Scripts/ParticleEmitterScript.h"

class PanScript : public MachineScript
{
    float m_BaseCookTime = 4.0f;
    float m_ExtraBaconTime = 2.0f;

public:
    void OnCreate() override
    {
        MachineScript::OnCreate();
        m_CookTime = m_BaseCookTime;

        auto* scriptComp = GetComponent<NativeScriptComponent>();
        if (scriptComp)
        {
            for (auto& s : scriptComp->Scripts)
            {
                if (s.Name == "SteamEmitterScript" && s.Instance)
                {
                    auto* emitter = static_cast<ParticleEmitterScript*>(s.Instance);
                    emitter->ParticleTemplate.PositionOffset = { 0.0f, 0.15f, 0.0f };
                    emitter->ParticleTemplate.PositionVariation = { 0.6f, 0.0f, 0.6f };
                    emitter->ParticleTemplate.SizeBegin = 1.0f;
                    emitter->ParticleTemplate.SizeVariation = 0.4f;
                    break;
                }
            }
        }

        SetSmoking(false);
    }

    void OnUpdate(Timestep ts) override
    {
        MachineScript::OnUpdate(ts);

        if (m_IsHeld) return;

        bool hasEgg = HasIngredient(IngredientType::Egg);
        bool hasHam = HasIngredient(IngredientType::ChoppedHam);
        bool hasTomato = HasIngredient(IngredientType::ChoppedTomato);

        if (hasEgg && !m_IsReady)
        {
            m_CurrentTime += ts.GetSeconds();
            float requiredTime = m_BaseCookTime;
            if (hasHam) requiredTime += m_ExtraBaconTime;
            if (hasTomato) requiredTime += 2.0f;

            if (m_CurrentTime >= requiredTime)
            {
                m_IsReady = true;
                SetSmoking(false);
                AudioEngine::Play("assets://sounds/dish_ready.mp3");
                UpdateVisuals();
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

    bool CanAcceptIngredient(IngredientType type) override
    {
        if (m_Ingredients.size() >= 2) return false;

        bool hasEgg = HasIngredient(IngredientType::Egg);
        bool hasHam = HasIngredient(IngredientType::ChoppedHam);
        bool hasTomato = HasIngredient(IngredientType::ChoppedTomato);

        if (type == IngredientType::Egg && !hasEgg) return true;
        if (type == IngredientType::ChoppedHam && !hasHam && !hasTomato) return true;
        if (type == IngredientType::ChoppedTomato && !hasTomato && !hasHam) return true;

        return false;
    }

    bool AddIngredient(IngredientType type) override
    {
        if (!CanAcceptIngredient(type)) return false;

        m_Ingredients.push_back(type);

        if (m_IsReady)
        {
            m_IsReady = false;
            auto* mesh = GetComponent<MeshComponent>();
            if (mesh) mesh->ModelPtr = AssetManager::GetModel("assets://models/przybory_kuchenne/patelka/pan.gltf");
            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max())
            {
                GetScene()->DestroyEntity(m_SpawnedFood);
                m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
            }
        }

        UpdateVisuals();
        AudioEngine::Play("assets://sounds/put_in_pot.mp3");

        if (HasIngredient(IngredientType::Egg)) {
            SetSmoking(true);
            AudioEngine::Play("assets://sounds/cooking.mp3");
        }
        else {
            SetSmoking(false);
        }

        return true;
    }

protected:
    void UpdateVisuals() override
    {
        if (m_IsReady)
        {
            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max())
                return;

            bool hasHam = HasIngredient(IngredientType::ChoppedHam);
            bool hasTomato = HasIngredient(IngredientType::ChoppedTomato);

            IngredientType type = IngredientType::FriedEgg;
            if (hasHam) type = IngredientType::EggWithHam;
            else if (hasTomato) type =  IngredientType::Shakshuka;

            auto* myTransform = GetComponent<TransformComponent>();
            if (!myTransform) return;

            m_SpawnedFood = SpawnMachineFood(type, "Na_Patelni");

            auto* foodTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);
            if (foodTf)
            {
                glm::vec3 baseScale = myTransform->GetScale();
                foodTf->SetScale(baseScale * 0.5f);
                foodTf->SetPosition(myTransform->GetPosition() + glm::vec3(0.0f, 0.23f, 0.0f));
            }

            auto* mesh = GetComponent<MeshComponent>();
            if (mesh) mesh->ModelPtr = nullptr;

            GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                    m_SpawnedFood, glm::vec3(1.0f, 0.2f, 0.6f), 1.5f, false
                });
            GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                    m_Entity, glm::vec3(1.0f, 0.2f, 0.6f), 1.5f, false
                });

            DishHistory history;
            history.BaseIngredients = m_Ingredients;
            history.OriginMachine = "Pan";
            GetScene()->GetWorld().GetEventBus().Publish(DishCreatedEvent{ m_SpawnedFood, history });
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
        if (m_SpawnedFood.id == std::numeric_limits<std::size_t>::max()) return;

        auto* foodTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);
        auto* plateTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(plate);
        auto* myTransform = GetComponent<TransformComponent>();
        auto* foodTag = GetScene()->GetWorld().GetComponent<TagComponent>(m_SpawnedFood);

        if (foodTransform && plateTransform && myTransform)
        {
            glm::vec3 plateScale = plateTransform->GetScale();
            glm::vec3 myScale = myTransform->GetScale();

            foodTransform->SetScale((myScale * 0.5f) / plateScale);
            foodTransform->SetPosition(glm::vec3(0.0f, 0.2f, 0.0f));

            GetScene()->SetParent(m_SpawnedFood, plate);

            if (foodTag)
                foodTag->Tag = "UgotowaneDanie";

            AudioEngine::Play("assets://sounds/plate_down.wav");

            GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                m_SpawnedFood, glm::vec3(0.2f, 1.0f, 0.2f), 1.5f, false
                });
            GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                plate, glm::vec3(0.2f, 1.0f, 0.2f), 1.5f, false
                });
        }

        m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
    }

    void ResetMachineState() override
    {
        MachineScript::ResetMachineState();
        SetSmoking(false, true);

        auto* mesh = GetComponent<MeshComponent>();
        if (mesh)
        {
            mesh->ModelPtr = AssetManager::GetModel("assets://models/przybory_kuchenne/patelka/pan.gltf");
        }
    }

private:
    bool HasIngredient(IngredientType type)
    {
        for (auto& i : m_Ingredients)
        {
            if (i == type) return true;
        }
        return false;
    }

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
                    if (state) emitter->Play();
                    else emitter->Stop();
                    if (clearInstatly) emitter->Clear();
                    break;
                }
            }
        }
    }
};