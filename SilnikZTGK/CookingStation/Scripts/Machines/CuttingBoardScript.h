#pragma once
#include "CookingStation/Layers/AssetLayer/AssetManager.h"

class CuttingBoardScript : public MachineScript
{
private:
    int m_ChopCount = 0;
    const int m_ChopsRequired = 3;
    Entity m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
    float m_ChopCooldown = 0.0f;

    float m_VisualJumpY = 0.0f;
    const float m_BaseYOffset = 0.7f;

    float m_AutoChopTimer = 0.0f;
    const float m_AutoChopInterval = 0.8f;

    std::pair<std::string, std::string> GetModelPathsForIngredient(IngredientType type)
    {
        switch (type)
        {
        case IngredientType::Tomato:
            return { "assets://models/skladniki/pomidor/pomidor.gltf", "assets://models/skladniki/pomidor/pomidor-pokrojony.gltf" };
        case IngredientType::Baguette:
            return { "assets://models/skladniki/bagietka/bagietka.gltf", "assets://models/skladniki/bagietka/bagietka-przekrojona.gltf" };
        case IngredientType::Cheese:
            return { "assets://models/skladniki/ser/ser.gltf", "assets://models/skladniki/ser/ser-pokrojony.gltf" };
        case IngredientType::Ham:
            return { "assets://models/skladniki/szynka/szynka.gltf", "assets://models/skladniki/szynka/szynka-pokrojona.gltf" };
        case IngredientType::Mozzarella:
            return { "assets://models/skladniki/pomidor/mozzarella.gltf", "assets://models/skladniki/pomidor/mozzarella-pokrojona.gltf" };
        default:
            return { "", "" };
        }
    }

    void PerformChop()
    {
        m_ChopCount++;
        m_ChopCooldown = 0.2f;
        spdlog::info("Ciach! ({}/{})", m_ChopCount, m_ChopsRequired);

        m_VisualJumpY = 0.3f;

        if (m_ChopCount >= m_ChopsRequired)
        {
            m_IsReady = true;
            m_VisualJumpY = 0.0f;
            m_AutoChopTimer = 0.0f;
            UpdateVisuals();
        }
    }

    void ResetMachineState() override
    {
        m_ChopCount = 0;
        m_AutoChopTimer = 0.0f;
        m_VisualJumpY = 0.0f;
        m_ChopCooldown = 0.0f;
        MachineScript::ResetMachineState();
    }

public:
    void OnUpdate(Timestep ts) override
    {
        MachineScript::OnUpdate(ts);
        if (m_IsHeld) return;

        if (m_ChopCooldown > 0.0f) {
            m_ChopCooldown -= ts.GetSeconds();
        }

        // --- AUTOMATYCZNE KROJENIE PRZEZ HELPERA ---
        if (m_IsAutomated && !m_IsReady && !m_Ingredients.empty())
        {
            m_AutoChopTimer += ts.GetSeconds();
            if (m_AutoChopTimer >= m_AutoChopInterval)
            {
                m_AutoChopTimer = 0.0f;
                PerformChop();
            }
        }
        else if (!m_IsAutomated || m_IsReady)
        {
            m_AutoChopTimer = 0.0f;
        }

        // --- GRAWITACJA PODSKOKU ---
        if (m_VisualJumpY > 0.0f) {
            float gravityPower = 5.0f;
            m_VisualJumpY -= gravityPower * ts.GetSeconds();
            if (m_VisualJumpY < 0.0f) m_VisualJumpY = 0.0f;
        }

        // --- APLIKACJA POZYCJI ---
        if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max()) {
            auto* boardTf = GetComponent<TransformComponent>();
            auto* foodTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);
            if (boardTf && foodTf) {
                glm::vec3 foodPos = boardTf->GetPosition();
                foodPos.y += m_BaseYOffset + m_VisualJumpY;
                foodTf->SetPosition(foodPos);
            }
        }

        if (Input::IsMouseButtonJustPressed(0) && !Input::IsKeyPressed(340))
        {
            glm::vec3 mousePos = GetMouseWorldPosition();
            auto* tf = GetComponent<TransformComponent>();
            if (!tf) return;

            glm::vec2 mousePos2D = { mousePos.x, mousePos.z };
            glm::vec2 boardPos2D = { tf->GetPosition().x, tf->GetPosition().z };

            if (glm::distance(mousePos2D, boardPos2D) < 3.0f)
            {
                if (m_IsReady)
                {
                    spdlog::info("Wziêto pokrojony sk³adnik z deski{}!", m_IsAutomated ? " (helper kroi³)" : "");

                    IngredientType rawType = m_Ingredients[0];
                    auto paths = GetModelPathsForIngredient(rawType);

                    if (rawType == IngredientType::Tomato) {
                        GetScene()->GetWorld().GetEventBus().Publish(
                            StartDragRequestEvent{ IngredientType::ChoppedTomato, paths.second }
                        );
                        spdlog::info("Event podnoszenia pomidora z deski wyslany");
                    }
                    else if (rawType == IngredientType::Baguette) {
                        GetScene()->GetWorld().GetEventBus().Publish(
                            StartDragRequestEvent{ IngredientType::CutBaguette, paths.second }
                        );
                    }
                    else if (rawType == IngredientType::Cheese) {
                        GetScene()->GetWorld().GetEventBus().Publish(
                            StartDragRequestEvent{ IngredientType::ChoppedCheese, paths.second }
                        );
                    }
                    else if (rawType == IngredientType::Ham) {
                        GetScene()->GetWorld().GetEventBus().Publish(
                            StartDragRequestEvent{ IngredientType::ChoppedHam, paths.second }
                        );
                    }
                    else if (rawType == IngredientType::Mozzarella) {
                        GetScene()->GetWorld().GetEventBus().Publish(
                            StartDragRequestEvent{ IngredientType::ChoppedMozzarella, paths.second }
                        );
                    }

                    ResetMachineState();
                }
                else if (!m_IsAutomated && !m_Ingredients.empty() && m_ChopCooldown <= 0.0f)
                {
                    PerformChop();
                }
            }
        }
    }

    bool AddIngredient(IngredientType type) override
    {
        if (m_IsReady || !m_Ingredients.empty()) return false;

        // ZMIANA: Deska teraz akceptuje wszystkie te sk³adniki!
        if (type == IngredientType::Tomato ||
            type == IngredientType::Baguette ||
            type == IngredientType::Cheese ||
            type == IngredientType::Ham ||
            type == IngredientType::Mozzarella)
        {
            m_Ingredients.push_back(type);
            m_ChopCount = 0;
            m_IsReady = false;
            m_ChopCooldown = 0.2f;
            m_AutoChopTimer = 0.0f;
            UpdateVisuals();
            spdlog::info("Po³o¿ono sk³adnik na desce do krojenia.");
            return true;
        }

        spdlog::warn("Deska: Tego sk³adnika tu nie pokroisz!");
        return false;
    }

protected:
    void TryTransferToPlate() override {}

    void UpdateVisuals() override
    {
        auto* myTransform = GetComponent<TransformComponent>();
        if (!myTransform) return;

        if (m_Ingredients.empty())
        {
            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max()) {
                GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_SpawnedFood });
                m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
            }
            return;
        }

        // Pobieramy œcie¿ki dla sk³adnika, który obecnie le¿y na desce
        auto paths = GetModelPathsForIngredient(m_Ingredients[0]);
        std::string currentModelPath = m_IsReady ? paths.second : paths.first;

        IngredientMetadata meta = GetIngredientMetadata(m_Ingredients[0]);

        if (m_SpawnedFood.id == std::numeric_limits<std::size_t>::max())
        {
            auto builder = GetScene()->GetWorld().BuildEntity();

            TransformComponent tc;
            tc.SetScale(meta.scale);
            tc.SetRotation(meta.rotation);

            builder.With<TransformComponent>(tc);

            MeshComponent mesh;
            mesh.ModelPtr = AssetManager::GetModel(currentModelPath);
            builder.With<MeshComponent>(mesh);
            m_SpawnedFood = builder.Build();
        }
        else
        {
            auto* mesh = GetScene()->GetWorld().GetComponent<MeshComponent>(m_SpawnedFood);
            if (mesh)
            {
                mesh->ModelPtr = AssetManager::GetModel(currentModelPath);
            }
        }
    }
};