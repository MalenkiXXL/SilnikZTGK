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
        // Shift+klik (pickup) obsługuje baza
        MachineScript::HandleClick();

        // Transfer tylko gdy baza NIE zrobiła go już za nas
        // (baza po naszej zmianie tego nie robi, więc robimy tu sami z warunkiem dystansu)
        if (m_IsReady && !m_IsAutomated && !m_IsHeld)
        {
            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max())
            {
                auto* foodTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);
                if (foodTransform)
                {
                    glm::vec3 mousePos = GetMouseWorldPosition();
                    glm::vec2 mousePos2D = { mousePos.x, mousePos.z };
                    glm::vec2 foodPos2D = {
                        foodTransform->GetPosition().x,
                        foodTransform->GetPosition().z
                    };

                    if (glm::distance(mousePos2D, foodPos2D) < 1.0f)
                    {
                        spdlog::info("Kliknieto w danie - przenosimy na talerz!");
                        ClearHighlight(); // <-- zgaś podświetlenie przed transferem
                        TryTransferToPlate();
                    }
                }
            }
        }
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

            auto builder = GetScene()->GetWorld().BuildEntity();
            builder.With<TagComponent>({ "W_Garnku" });

            TransformComponent tc;
            tc.SetPosition(myTransform->GetPosition() + glm::vec3(0.0f, 1.0f, 0.0f));
            tc.SetScale(glm::vec3(0.7f, 0.7f, 0.7f));
            builder.With<TransformComponent>(tc);

            MeshComponent mesh;
            mesh.ModelPtr = AssetManager::GetModel("CookingStation/Assets/models/skladniki/pomidor/pomidorowa.gltf");
            builder.With<MeshComponent>(mesh);

            m_SpawnedFood = builder.Build();
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
        if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max())
        {
            auto* foodTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);
            auto* foodTag = GetScene()->GetWorld().GetComponent<TagComponent>(m_SpawnedFood);

            if (foodTransform)
            {
                foodTransform->SetPosition(glm::vec3(0.0f, 0.15f, 0.0f));
                GetScene()->SetParent(m_SpawnedFood, plate);

                if (foodTag)
                    foodTag->Tag = "UgotowaneDanie";

                spdlog::info("Gotowe jedzenie podane na talerz - czeka na kelnera!");
            }

            m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
        }
    }
};