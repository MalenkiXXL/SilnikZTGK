#pragma once
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include "CookingStation/Scripts/DragAndDropScript.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"

class MixerScript : public MachineScript
{
private:
    Entity m_SpawnedDough = { std::numeric_limits<std::size_t>::max(), 0 };
    std::size_t m_DoughClickSubId = 0;

public:
    void OnCreate() override
    {
        MachineScript::OnCreate();
        m_CookTime = 4.0f;

        m_DoughClickSubId = GetScene()->GetWorld().GetEventBus().Subscribe<EntityClickedEvent>(
            [this](const EntityClickedEvent& e) {
                if (e.TargetEntity.id != std::numeric_limits<std::size_t>::max() &&
                    m_SpawnedDough.id != std::numeric_limits<std::size_t>::max() &&
                    e.TargetEntity.id == m_SpawnedDough.id && m_IsReady)
                {
                    GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_SpawnedDough });
                    m_SpawnedDough = { std::numeric_limits<std::size_t>::max(), 0 };
                    DragAndDropScript::StartDrag(IngredientType::RawDough, "assets://models/skladniki/maka/maka.gltf");
                    ResetMachineState();
                }
            }
        );
    }

    void OnDestroy() override
    {
        GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityClickedEvent>(m_DoughClickSubId);
        MachineScript::OnDestroy();
    }

    void OnUpdate(Timestep ts) override
    {
        MachineScript::OnUpdate(ts);
        if (m_IsHeld) return;

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
            return true;
        }

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
            tc.SetPosition(myTransform->GetPosition() + glm::vec3(0.0f, 1.2f, 0.0f));
            IngredientMetadata meta = GetIngredientMetadata(IngredientType::RawDough);
            tc.SetScale(meta.scale);
            tc.SetRotation(meta.rotation);
            builder.With<TransformComponent>(tc);

            MeshComponent mesh;
            mesh.ModelPtr = AssetManager::GetModel("assets://models/skladniki/maka/maka.gltf");
            builder.With<MeshComponent>(mesh);

            BoxColliderComponent collider;
            collider.Size = glm::vec3(1.2f);
            builder.With<BoxColliderComponent>(collider);

            m_SpawnedDough = builder.Build();
        }
        else
        {
            if (m_SpawnedDough.id != std::numeric_limits<std::size_t>::max())
            {
                GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_SpawnedDough });
                m_SpawnedDough = { std::numeric_limits<std::size_t>::max(), 0 };
            }
        }
    }
};