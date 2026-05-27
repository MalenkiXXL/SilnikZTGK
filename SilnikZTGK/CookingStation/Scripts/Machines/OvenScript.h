#pragma once
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include "CookingStation/Scripts/DragAndDropScript.h"

class OvenScript : public MachineScript
{
private:
    Entity m_SpawnedBread = { std::numeric_limits<std::size_t>::max(), 0 };

public:
    void OnCreate() override
    {
        m_CookTime = 8.0f; // Pieczenie trwa trochê d³u¿ej
    }

    void OnUpdate(Timestep ts) override
    {
        MachineScript::OnUpdate(ts);
        if (m_IsHeld) return;

        if (m_IsReady && Input::IsMouseButtonJustPressed(0))
        {
            if (m_SpawnedBread.id != std::numeric_limits<std::size_t>::max())
            {
                auto* transform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedBread);
                if (transform)
                {
                    glm::vec3 mousePos = GetMouseWorldPosition();
                    glm::vec2 mousePos2D = { mousePos.x, mousePos.z };
                    glm::vec2 foodPos2D = { transform->GetPosition().x, transform->GetPosition().z };

                    if (glm::distance(mousePos2D, foodPos2D) < 3.0f)
                    {
                        // Gracz otrzymuje upieczon¹ bagietkê
                        DragAndDropScript::StartDrag(IngredientType::Baguette, "assets://models/skladniki/bagietka/bagietka.gltf");
                        ResetMachineState();
                    }
                }
            }
        }

        // Logika Pieczenia
        if (!m_Ingredients.empty() && !m_IsReady)
        {
            m_CurrentTime += ts.GetSeconds();
            if (m_CurrentTime >= m_CookTime)
            {
                m_IsReady = true;
                UpdateVisuals();
            }
        }
    }

    bool AddIngredient(IngredientType type) override
    {
        if (m_IsReady || !m_Ingredients.empty()) return false;

        if (type == IngredientType::RawDough)
        {
            m_Ingredients.push_back(type);
            m_IsReady = false;
            m_CurrentTime = 0.0f;
            spdlog::info("Piekarnik: Rozpoczêto pieczenie!");
            return true;
        }

        spdlog::warn("Piekarnik: Do piekarnika wrzucaj tylko wyrobione ciasto!");
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
            builder.With<TagComponent>({ "BagietkaWPiekarniku" });

            TransformComponent tc;
            tc.SetPosition(myTransform->GetPosition() + glm::vec3(0.0f, 1.0f, 0.0f));
            IngredientMetadata meta = GetIngredientMetadata(IngredientType::RawDough);
            tc.SetScale(meta.scale);
            tc.SetRotation(meta.rotation);
            builder.With<TransformComponent>(tc);

            MeshComponent mesh;
            mesh.ModelPtr = AssetManager::GetModel("assets://models/skladniki/bagietka/bagietka.gltf");
            builder.With<MeshComponent>(mesh);

            m_SpawnedBread = builder.Build();
        }
        else
        {
            if (m_SpawnedBread.id != std::numeric_limits<std::size_t>::max())
            {
                auto* tf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedBread);
                if (tf) tf->SetPosition(glm::vec3(0.0f, -1000.0f, 0.0f));
                m_SpawnedBread = { std::numeric_limits<std::size_t>::max(), 0 };
            }
        }
    }
};