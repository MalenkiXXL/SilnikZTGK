#pragma once
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scripts/DragAndDropScript.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Core/Application.h"
#include <GLFW/glfw3.h>

class CuttingBoardScript : public MachineScript
{
private:
    const int m_ChopsRequired = 3;
    float m_ChopCooldown = 0.0f;

    float m_VisualJumpY = 0.0f;
    const float m_BaseYOffset = 0.05f;

    float m_AutoChopTimer = 0.0f;
    const float m_AutoChopInterval = 0.8f;

    Entity m_CursorKnife = { std::numeric_limits<std::size_t>::max(), 0 };

    bool m_WasShowingKnife = false;

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
        case IngredientType::Apple:
            return { "assets://models/skladniki/jablko/apple1.gltf", "assets://models/skladniki/pomidor/pomidor-pokrojony.gltf" };
        case IngredientType::Raspberry:
            return { "assets://models/skladniki/malina/malina.gltf", "assets://models/skladniki/szynka/szynka-pokrojona.gltf" };
        default:
            return { "", "" };
        }
    }

    IngredientType GetChoppedType(IngredientType rawType)
    {
        switch (rawType) {
        case IngredientType::Tomato: return IngredientType::ChoppedTomato;
        case IngredientType::Baguette: return IngredientType::CutBaguette;
        case IngredientType::Cheese: return IngredientType::ChoppedCheese;
        case IngredientType::Ham: return IngredientType::ChoppedHam;
        case IngredientType::Mozzarella: return IngredientType::ChoppedMozzarella;
        case IngredientType::Apple: return IngredientType::ChoppedApple;
        case IngredientType::Raspberry: return IngredientType::ChoppedRaspberry;
        default: return IngredientType::None;
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
    int m_ChopCount = 0;

    void PerformChop()
    {
        if (m_ChopCooldown > 0.0f) return;

        m_ChopCount++;
        m_ChopCooldown = 0.2f;
        spdlog::info("Ciach! ({}/{})", m_ChopCount, m_ChopsRequired);
        AudioEngine::Play("assets://sounds/chop.wav");

        m_VisualJumpY = 0.3f;

        if (m_ChopCount >= m_ChopsRequired)
        {
            m_IsReady = true;
            m_VisualJumpY = 0.0f;
            m_AutoChopTimer = 0.0f;
            UpdateVisuals();
        }
    }

    void OnCreate() override
    {
        MachineScript::OnCreate();


        GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityClickedEvent>(m_ClickSubId);
        GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityClickedEvent>(m_FoodClickSubId);
        GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityHoveredEvent>(m_HoverSubId);
    }

    void OnDestroy() override
    {
        if (m_WasShowingKnife)
        {
            GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            m_WasShowingKnife = false;
        }

        if (m_CursorKnife.id != std::numeric_limits<std::size_t>::max())
        {
            GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_CursorKnife });
        }

        m_ClickSubId = 0;
        m_FoodClickSubId = 0;
        m_HoverSubId = 0;

        MachineScript::OnDestroy();
    }

    void TryTransferToPlate() override
    {
        if (m_ChopCooldown > 0.0f) return;

        Entity targetPlate = GetClosestAvailablePlate();

        if (targetPlate.id != std::numeric_limits<std::size_t>::max())
        {
            auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(targetPlate);
            PlateScript* pScript = nullptr;
            if (nsc) {
                for (auto& s : nsc->Scripts) {
                    if (s.Name == "PlateScript" && s.Instance) {
                        pScript = static_cast<PlateScript*>(s.Instance);
                        break;
                    }
                }
            }

            if (pScript)
            {
                IngredientType choppedType = GetChoppedType(m_Ingredients[0]);

                if (pScript->AddIngredient(choppedType))
                {
                    spdlog::info("Składnik z deski przeniesiony na talerz!");
                    ClearHighlight();
                    ResetMachineState();
                }
                else
                {
                    spdlog::warn("Talerz jest pełny lub nie może przyjąć składnika!");
                }
            }
        }
        else if (!m_IsAutomated)
        {
            if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max())
            {
                GetScene()->DestroyEntity(m_SpawnedFood);
                m_SpawnedFood = { std::numeric_limits<std::size_t>::max(), 0 };

                IngredientType choppedType = GetChoppedType(m_Ingredients[0]);
                DragAndDropScript::StartDrag(choppedType);
                ResetMachineState();
                ClearHighlight();
            }
            else
            {
                spdlog::warn("Brak talerza w promieniu kratki - nie można nałożyć!");
            }
        }
    }

    void OnUpdate(Timestep ts) override
    {
        MachineScript::OnUpdate(ts);

        if (m_ChopCooldown > 0.0f) {
            m_ChopCooldown -= ts.GetSeconds();
        }

        if (m_SpawnedFood.id != std::numeric_limits<std::size_t>::max()) {
            auto* boardTf = GetComponent<TransformComponent>();
            auto* foodTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);
            if (boardTf && foodTf) {
                glm::vec3 foodPos = boardTf->GetPosition();
                foodPos.y += m_BaseYOffset + m_VisualJumpY;
                foodTf->SetPosition(foodPos);
            }
        }

        if (m_IsHeld) return;

        if (m_VisualJumpY > 0.0f) {
            float gravityPower = 5.0f;
            m_VisualJumpY -= gravityPower * ts.GetSeconds();
            if (m_VisualJumpY < 0.0f) m_VisualJumpY = 0.0f;
        }

        if (m_IsAutomated) {
            if (!m_IsReady && !m_Ingredients.empty()) {
                m_AutoChopTimer += ts.GetSeconds();
                if (m_AutoChopTimer >= m_AutoChopInterval) {
                    m_AutoChopTimer = 0.0f;
                    PerformChop();
                }
            }
            else if (m_IsReady) {
                TryTransferToPlate();
            }
            return;
        }

        auto* tf = GetComponent<TransformComponent>();
        if (!tf) return;

        glm::vec3 floorMousePos = GetMouseWorldPosition();

        Camera* camera = GetScene()->GetCamera();

        glm::vec3 preciseMousePos = floorMousePos;

        if (camera)
        {
            float targetY = tf->GetPosition().y;
            glm::vec3 rayDir = camera->Front;

            if (std::abs(rayDir.y) > 0.001f)
            {
                float t = (targetY - floorMousePos.y) / rayDir.y;

                preciseMousePos = floorMousePos + rayDir * t;
            }
        }

        glm::vec2 mouse2D = { preciseMousePos.x, preciseMousePos.z };
        glm::vec2 board2D = { tf->GetPosition().x, tf->GetPosition().z };

        bool isHovering = (glm::distance(mouse2D, board2D) < 1.5f);

        if (isHovering && m_IsReady && !m_IsAutomated && !GlobalIsMachineHeld && !m_IsHeld)
        {
            Entity closestPlate = GetClosestAvailablePlate();
            if (closestPlate.id != std::numeric_limits<std::size_t>::max())
            {
                SetPlateHighlight(closestPlate, true);
            }
        }

        bool shouldShowKnife = isHovering && !m_IsAutomated && !m_IsReady && !m_Ingredients.empty() && !GlobalIsMachineHeld;
        if (shouldShowKnife)
        {
            if (!m_WasShowingKnife)
            {
                // Ukrywa systemowy kursor przez natywne wywołanie GLFW
                GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
                m_WasShowingKnife = true;
            }

            if (m_CursorKnife.id == std::numeric_limits<std::size_t>::max())
            {
                auto builder = GetScene()->GetWorld().BuildEntity();
                builder.With<TagComponent>({ "Noz" });

                TransformComponent tc;
                tc.SetScale(glm::vec3(1.0f));
                builder.With<TransformComponent>(tc);

                MeshComponent mesh;
                mesh.ModelPtr = AssetManager::GetModel("assets://models/przybory_kuchenne/noz/knife.gltf");
                builder.With<MeshComponent>(mesh);

                m_CursorKnife = builder.Build();
            }

            auto* knifeTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_CursorKnife);
            if (knifeTf)
            {
                glm::vec3 knifePos = preciseMousePos;
                knifePos.y = tf->GetPosition().y + m_BaseYOffset + 1.f;

                float offsetX = 1.7f;
                float offsetZ = 1.7f;

                knifePos.x += offsetX;
                knifePos.z += offsetZ;

                knifeTf->SetPosition(knifePos);
            }
        }
        else
        {
            if (m_WasShowingKnife)
            {
                GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                m_WasShowingKnife = false;
            }

            if (m_CursorKnife.id != std::numeric_limits<std::size_t>::max())
            {
                GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_CursorKnife });
                m_CursorKnife = { std::numeric_limits<std::size_t>::max(), 0 };
            }
        }

        bool isMouseClick = Input::IsMouseButtonJustPressed(0);
        bool isGamepadTransfer = Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0);
        bool isGamepadChop = Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(3, 0);

        if ((isMouseClick || isGamepadTransfer || isGamepadChop) && isHovering && !Input::IsUICapturingMouse()) {
            if (Input::IsKeyPressed(340))
            {
                if (!m_IsHeld && !GlobalIsMachineHeld)
                {
                    m_IsHeld = true;
                    m_IsNewlySpawned = false;
                    GlobalIsMachineHeld = true;
                    m_PickupDelay = 0.2f;
                    m_OriginalPosition = tf->GetPosition();
                    ClearHighlight();
                }
            }
            else
            {
                if (m_IsReady)
                {
                    if (isMouseClick || isGamepadTransfer) TryTransferToPlate();
                }
                else if (!m_Ingredients.empty() && m_ChopCooldown <= 0.0f)
                {
                    if (isMouseClick || isGamepadChop) PerformChop();
                }
            }
        }
        m_IsHoveredThisFrame = false;
    }

    virtual void HandleClick() override {}

    bool AddIngredient(IngredientType type) override
    {
        if (m_IsReady || !m_Ingredients.empty()) return false;

        if (type == IngredientType::Tomato ||
            type == IngredientType::Baguette ||
            type == IngredientType::Cheese ||
            type == IngredientType::Ham ||
            type == IngredientType::Mozzarella ||
            type == IngredientType::Apple ||
            type == IngredientType::Raspberry)
        {
            m_Ingredients.push_back(type);
            m_ChopCount = 0;
            m_IsReady = false;
            m_ChopCooldown = 0.2f;
            m_AutoChopTimer = 0.0f;
            UpdateVisuals();
            spdlog::info("Położono składnik na desce do krojenia.");
            return true;
        }

        spdlog::warn("Deska: Tego składnika tu nie pokroisz!");
        return false;
    }

protected:

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

        auto paths = GetModelPathsForIngredient(m_Ingredients[0]);
        std::string currentModelPath = m_IsReady ? paths.second : paths.first;

        IngredientType visualType = m_IsReady ? GetChoppedType(m_Ingredients[0]) : m_Ingredients[0];

        if (m_SpawnedFood.id == std::numeric_limits<std::size_t>::max())
        {
            m_SpawnedFood = SpawnMachineFood(visualType, "Na_Desce");
        }
        else
        {
            auto* mesh = GetScene()->GetWorld().GetComponent<MeshComponent>(m_SpawnedFood);
            if (mesh)
            {
                mesh->ModelPtr = AssetManager::GetModel(currentModelPath);
            }

            auto* foodTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_SpawnedFood);
            if (foodTf)
            {
                IngredientMetadata meta = GetIngredientMetadata(visualType);
                foodTf->SetScale(meta.scale);
                foodTf->SetRotation(meta.rotation);
            }
        }

        if (m_IsReady)
        {
            DishHistory history;
            history.BaseIngredients = m_Ingredients;
            history.OriginMachine = "CuttingBoard";
            GetScene()->GetWorld().GetEventBus().Publish(DishCreatedEvent{ m_SpawnedFood, history });
            spdlog::info("Składnik pokrojony i wpisany do rejestru historii.");
        }
    }
};