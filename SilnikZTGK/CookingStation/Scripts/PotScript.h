#pragma once
#include "MachineScript.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h" // POTRZEBNE DO MODELU KANAPKI

class PotScript : public MachineScript
{
private:
    // Pamiêtamy wygenerowan¹ kanapkê, ¿eby móc j¹ zniszczyæ lub przenieœæ na talerz
    Entity m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };

public:
    void OnCreate() override
    {
        m_CookTime = 3.0f; // Zmniejszone do 3 sekund dla szybszych testów!
    }

    void OnUpdate(Timestep ts) override
    {
        MachineScript::OnUpdate(ts);

        if (m_IsHeld) return;

        // --- RÊCZNE KLIKANIE W KANAPKÊ ---
        // Jeœli jest gotowe i w³aœnie wciœniêto lewy przycisk myszy
        if (m_IsReady && Input::IsMouseButtonJustPressed(0))
        {
            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max())
            {
                auto* foodTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);
                if (foodTransform)
                {
                    glm::vec3 mousePos = GetMouseWorldPosition();

                    // Ignorujemy wysokoœæ (Y), ¿eby ³atwo trafiaæ w obiekt z góry kamer¹
                    glm::vec2 mousePos2D = { mousePos.x, mousePos.z };
                    glm::vec2 foodPos2D = { foodTransform->GetPosition().x, foodTransform->GetPosition().z };

                    // Skoro kanapka ma skalê 7.0, promieñ klikniêcia 3.0f bêdzie idealny
                    if (glm::distance(mousePos2D, foodPos2D) < 3.0f)
                    {
                        spdlog::info("Kliknieto prosto w apetyczna kanapke!");
                        TryTransferToPlate();
                    }
                }
            }
        }
        // ---------------------------------

        // LOGIKA GOTOWANIA
        if (!m_Ingredients.empty() && !m_IsReady)
        {
            m_CurrentTime += ts.GetSeconds();

            if (m_CurrentTime >= m_CookTime)
            {
                m_IsReady = true;
                UpdateVisuals();
            }
        }

        // AUTOMATYZACJA
        if (m_IsAutomated && m_IsReady)
        {
            TryTransferToPlate();
        }
    }

    bool AddIngredient(IngredientType type)
    {
        if (m_IsReady || m_Ingredients.size() >= 2) return false;

        m_Ingredients.push_back(type);
        m_IsReady = false;
        m_CurrentTime = 0.0f;
        UpdateVisuals();

        return true;
    }

protected:
    void UpdateVisuals() override
    {
        if (m_IsReady)
        {
            // Zbuduj model kanapki!
            auto* myTransform = GetComponent<TransformComponent>();
            if (!myTransform) return;

            auto builder = GetScene()->GetWorld().BuildEntity();
            builder.With<TagComponent>({ "W_Garnku" });

            TransformComponent tc;
            tc.SetPosition(myTransform->GetPosition() + glm::vec3(0.0f, 1.0f, 0.0f)); // Unosi siê o 1 metr nad garnkiem
            tc.SetScale(glm::vec3(0.7f, 0.7f, 0.7f)); // Rozmiar kanapki
            builder.With<TransformComponent>(tc);

            MeshComponent mesh;
            mesh.ModelPtr = AssetManager::GetModel("CookingStation/Assets/models/skladniki/pomidor/pomidorowa.gltf");
            builder.With<MeshComponent>(mesh);

            m_SpawnedFood = builder.Build();
            spdlog::info("Zupa gotowa! Pojawila sie kanapka nad garnkiem.");
        }
        else
        {
            // Jeœli garnek zresetowano przed prze³o¿eniem na talerz (np. wyrzucono zupê do kosza), chowamy kanapkê
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

            // --- NOWOŒÆ: Pobieramy komponent tagu z kanapki ---
            auto* foodTag = GetScene()->GetWorld().GetComponent<TagComponent>(m_SpawnedFood);

            if (foodTransform)
            {
                foodTransform->SetPosition(glm::vec3(0.0f, 0.15f, 0.0f));
                GetScene()->SetParent(m_SpawnedFood, plate);

                // --- NOWOŒÆ: Zmieniamy tag na ten w³aœciwy! ---
                if (foodTag)
                {
                    foodTag->Tag = "UgotowaneDanie";
                }

                spdlog::info("Gotowe jedzenie zostalo przyklejone do talerza i czeka na kelnera!");
            }

            m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
        }
    }
};