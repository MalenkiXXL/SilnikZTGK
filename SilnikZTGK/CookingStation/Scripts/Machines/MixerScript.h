#pragma once
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include "CookingStation/Scripts/DragAndDropScript.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"

class MixerScript : public MachineScript
{
private:
    Entity m_SpawnedDough = { std::numeric_limits<std::size_t>::max(), 0 };

public:
    void OnCreate() override
    {
        m_CookTime = 4.0f; // Szybkie wyrabianie ciasta
    }

    void OnUpdate(Timestep ts) override
    {
        MachineScript::OnUpdate(ts);
        if (m_IsHeld) return;

        // Jeœli ciasto gotowe i gracz klikn¹³
        if (m_IsReady && Input::IsMouseButtonJustPressed(0))
        {
            if (m_SpawnedDough.id != std::numeric_limits<std::size_t>::max())
            {
                auto* transform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedDough);
                if (transform)
                {
                    glm::vec3 mousePos = GetMouseWorldPosition();
                    glm::vec2 mousePos2D = { mousePos.x, mousePos.z };
                    glm::vec2 foodPos2D = { transform->GetPosition().x, transform->GetPosition().z };

                    // Klikniêto w gotowe ciasto
                    if (glm::distance(mousePos2D, foodPos2D) < 3.0f)
                    {
                        spdlog::info("Mixer: Wyciagniêto wyrobione ciasto!");

                        // Przekazujemy surowe ciasto graczowi do r¹k (na myszkê)
                        DragAndDropScript::StartDrag(IngredientType::RawDough, "assets://models/skladniki/maka/maka.gltf", glm::vec3(0.8f)); // Zmieñ œcie¿kê, jeœli masz model surowego ciasta!

                        ResetMachineState();
                    }
                }
            }
        }

        // Logika miksowania (Startuje dopiero, gdy w œrodku jest m¹ka ORAZ mleko)
        if (!m_IsReady)
        {
            bool hasFlour = std::find(m_Ingredients.begin(), m_Ingredients.end(), IngredientType::Flour) != m_Ingredients.end();
            bool hasMilk = std::find(m_Ingredients.begin(), m_Ingredients.end(), IngredientType::Milk) != m_Ingredients.end();

            if (hasFlour && hasMilk)
            {
                m_CurrentTime += ts.GetSeconds();
                if (m_CurrentTime >= m_CookTime)
                {
                    m_IsReady = true;
                    UpdateVisuals();
                }
            }
        }
    }

    bool AddIngredient(IngredientType type) override
    {
        if (m_IsReady || m_Ingredients.size() >= 3) return false;

        if (type == IngredientType::Flour || type == IngredientType::Milk || type == IngredientType::Egg || type == IngredientType::Potato)
        {
            m_Ingredients.push_back(type);
            m_IsReady = false;
            m_CurrentTime = 0.0f;
            spdlog::info("Mixer: Przyjêto sk³adnik!");
            return true;
        }

        spdlog::warn("Mixer: Tego tu nie wrzucisz!");
        return false;
    }

protected:
    void UpdateVisuals() override
    {
        if (m_IsReady)
        {
            auto* myTransform = GetComponent<TransformComponent>();
            if (!myTransform) return;

            auto builder = GetScene()->GetWorld().BuildEntity();
            builder.With<TagComponent>({ "WyrobioneCiasto" });

            TransformComponent tc;
            tc.SetPosition(myTransform->GetPosition() + glm::vec3(0.0f, 1.0f, 0.0f)); // Ciasto unosi siê nad mis¹
            tc.SetScale(glm::vec3(8.0f));
            builder.With<TransformComponent>(tc);

            MeshComponent mesh;
            mesh.ModelPtr = AssetManager::GetModel("assets://models/skladniki/maka/maka.gltf"); // Zast¹p modelem surowego ciasta
            builder.With<MeshComponent>(mesh);

            m_SpawnedDough = builder.Build();
        }
        else
        {
            if (m_SpawnedDough.id != std::numeric_limits<std::size_t>::max())
            {
                auto* tf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedDough);
                if (tf) tf->SetPosition(glm::vec3(0.0f, -1000.0f, 0.0f));
                m_SpawnedDough = { std::numeric_limits<std::size_t>::max(), 0 };
            }
        }
    }
};