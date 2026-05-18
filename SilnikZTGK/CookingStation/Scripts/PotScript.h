#pragma once
#include "MachineScript.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h" 
#include "CookingStation/Core/GameProgress.h"

class PotScript : public MachineScript
{
private:
    // Pamiêtamy wygenerowane jedzenie, ¿eby móc je zniszczyæ lub przenieœæ na talerz
    Entity m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };

public:
    void OnCreate() override
    {
        m_CookTime = 3.0f; // Czas gotowania 
    }

    void OnUpdate(Timestep ts) override
    {
        MachineScript::OnUpdate(ts);

        if (m_IsHeld) return;

        // Jeœli jest gotowe i w³aœnie wciœniêto lewy przycisk myszy
        if (m_IsReady && Input::IsMouseButtonJustPressed(0))
        {
            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max())
            {
                auto* foodTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);
                if (foodTransform)
                {
                    glm::vec3 mousePos = GetMouseWorldPosition();

                    // Ignorujemy wysokoœæ, ¿eby ³atwo trafiaæ w obiekt z góry kamer¹
                    glm::vec2 mousePos2D = { mousePos.x, mousePos.z };
                    glm::vec2 foodPos2D = { foodTransform->GetPosition().x, foodTransform->GetPosition().z };

                    // Promieñ klikniêcia dopasowany do dania
                    if (glm::distance(mousePos2D, foodPos2D) < 3.0f)
                    {
                        spdlog::info("Kliknieto w danie!");
                        TryTransferToPlate();
                    }
                }
            }
        }

        // Logika gotowania
        if (!m_Ingredients.empty() && !m_IsReady)
        {
            m_CurrentTime += ts.GetSeconds();

            if (m_CurrentTime >= m_CookTime)
            {
                m_IsReady = true;
                UpdateVisuals();
            }
        }

        // Automatyzacja 
        if (m_IsAutomated && m_IsReady)
        {
            TryTransferToPlate();
        }
    }

    bool AddIngredient(IngredientType type) override
    {
        if (m_IsReady || m_Ingredients.size() >= 2) return false;

        // Przyjmuje tylko pokrojonego pomidora
        if (type == IngredientType::ChoppedTomato)
        {
            m_Ingredients.push_back(type);
            m_IsReady = false;
            m_CurrentTime = 0.0f;
            UpdateVisuals();
            spdlog::info("Garnek: Przyjêto pokrojonego pomidora, zaczynamy gotowanie!");
            return true;
        }

        spdlog::warn("Garnek: Nie wrzucaj tego! Najpierw pokrój na desce!");
        return false;
    }

protected:
    void UpdateVisuals() override
    {
        if (m_IsReady)
        {
            if (!GameProgress::IsRecipeUnlocked("TomatoSoup"))
            {
                GameProgress::UnlockRecipe("TomatoSoup");
                spdlog::info("Zupa pomidorowa odblokowana po raz pierwszy!");
            }

            // Zbuduj model dania
            auto* myTransform = GetComponent<TransformComponent>();
            if (!myTransform) return;

            auto builder = GetScene()->GetWorld().BuildEntity();
            builder.With<TagComponent>({ "W_Garnku" });

            TransformComponent tc;
            tc.SetPosition(myTransform->GetPosition() + glm::vec3(0.0f, 1.0f, 0.0f)); // Jak wysoko nad garnkiem
            tc.SetScale(glm::vec3(0.7f, 0.7f, 0.7f)); // Rozmiar dania
            builder.With<TransformComponent>(tc);

            MeshComponent mesh;
            mesh.ModelPtr = AssetManager::GetModel("CookingStation/Assets/models/skladniki/pomidor/pomidorowa.gltf");
            builder.With<MeshComponent>(mesh);

            m_SpawnedFood = builder.Build();
            spdlog::info("Danie gotowe, pojawia siê nad garnkiem.");
        }
        else
        {
            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max())
            {
                auto* tf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);
                if (tf) tf->SetPosition(glm::vec3(0.0f, -1000.0f, 0.0f)); // Pod mapê
                m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
            }
        }
    }

    void OnTransferToPlate(Entity plate) override
    {
        if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max())
        {
            auto* foodTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);

            // Pobieramy tag z dania 
            auto* foodTag = GetScene()->GetWorld().GetComponent<TagComponent>(m_SpawnedFood);

            if (foodTransform)
            {
                foodTransform->SetPosition(glm::vec3(0.0f, 0.15f, 0.0f));
                GetScene()->SetParent(m_SpawnedFood, plate);

                // Zmiana tagu na gotowy -> dla kelnera 
                if (foodTag)
                {
                    foodTag->Tag = "UgotowaneDanie";
                }

                spdlog::info("Gotowe jedzenie podane na talerz - czeka na kelnera!");
            }

            m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
        }
    }
};