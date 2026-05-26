#pragma once
#include "MachineScript.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h" 
#include "CookingStation/Core/GameProgress.h"
#include "CookingStation/Scripts/ParticleEmitterScript.h"

class PotScript : public MachineScript
{
private:
    Entity m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };

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
        MachineScript::OnCreate(); // Wazne: wywolujemy bazowe OnCreate zeby zasubskrybowac EntityClickedEvent!
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

    // Calkowite klikniecie przeniesione tu z OnUpdate - HandleClick jest wywolywane przez EntityClickedEvent
    virtual void HandleClick() override
    {
        if (!m_IsHeld && Input::IsKeyPressed(340))
        {
            // Shift+klik = podniesienie maszyny
            m_IsHeld = true;
            spdlog::info("Podniesiono garnek!");
            return;
        }

        if (m_IsReady && !m_IsAutomated && !m_IsHeld)
        {
            // Sprawdzamy czy klik trafil w danie nad garnkiem
            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max())
            {
                auto* foodTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);
                if (foodTransform)
                {
                    glm::vec3 mousePos = GetMouseWorldPosition();
                    glm::vec2 mousePos2D = { mousePos.x, mousePos.z };
                    glm::vec2 foodPos2D = { foodTransform->GetPosition().x, foodTransform->GetPosition().z };

                    if (glm::distance(mousePos2D, foodPos2D) < 3.0f)
                    {
                        spdlog::info("Kliknieto w danie - przenosimy na talerz!");
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
            // Guard: nie twórz dania jesli juz istnieje
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
                // Niszczymy encje zamiast chowac pod mape - koniec wycieku encji
                GetScene()->GetWorld().DestroyEntity(m_SpawnedFood);
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