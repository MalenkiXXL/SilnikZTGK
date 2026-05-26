#pragma once
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scripts/DragAndDropScript.h"

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

    // NOWE: Funkcja pomocnicza, która mówi, jaki model za³adowaæ dla jakiego sk³adnika
    // Zwraca parê: <œcie¿ka_do_surowego, œcie¿ka_do_pokrojonego>
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

                    // ZMIANA: Sprawdzamy co le¿a³o na desce i dobieramy typ/model/skalê!
                    IngredientType rawType = m_Ingredients[0];
                    auto paths = GetModelPathsForIngredient(rawType);

                    if (rawType == IngredientType::Tomato) {
                        DragAndDropScript::StartDrag(IngredientType::ChoppedTomato, paths.second, glm::vec3(0.6f));
                    }
                    else if (rawType == IngredientType::Baguette) {
                        DragAndDropScript::StartDrag(IngredientType::CutBaguette, paths.second, glm::vec3(1.0f));
                    }
                    else if (rawType == IngredientType::Cheese) {
                        DragAndDropScript::StartDrag(IngredientType::ChoppedCheese, paths.second, glm::vec3(1.5f), glm::vec3(glm::radians(90.0f), 0.0f, 0.0f));
                    }
                    else if (rawType == IngredientType::Ham) {
                        DragAndDropScript::StartDrag(IngredientType::ChoppedHam, paths.second, glm::vec3(1.8f), glm::vec3(glm::radians(-90.0f), 0.0f, glm::radians(180.0f)));
                    }
                    else if (rawType == IngredientType::Mozzarella) {
                        DragAndDropScript::StartDrag(IngredientType::ChoppedMozzarella, paths.second, glm::vec3(1.0f));
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
                auto* tf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);
                if (tf) tf->SetPosition(glm::vec3(0.0f, -1000.0f, 0.0f));
                m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };
            }
            return;
        }

        // Pobieramy œcie¿ki dla sk³adnika, który obecnie le¿y na desce
        auto paths = GetModelPathsForIngredient(m_Ingredients[0]);
        std::string currentModelPath = m_IsReady ? paths.second : paths.first;

        if (m_SpawnedFood.id == std::numeric_limits<std::size_t>::max())
        {
            auto builder = GetScene()->GetWorld().BuildEntity();

            TransformComponent tc;
            tc.SetPosition(myTransform->GetPosition() + glm::vec3(0.0f, m_BaseYOffset, 0.0f));

            // TODO: W przysz³oœci mo¿esz te¿ podpi¹æ skalê specyficzn¹ dla sk³adnika (tak jak w UI)
            tc.SetScale(glm::vec3(0.7f, 0.7f, 0.7f));

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