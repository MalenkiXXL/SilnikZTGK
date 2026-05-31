#pragma once
#include "CookingStation/Layers/AssetLayer/AssetManager.h"

class CuttingBoardScript : public MachineScript
{
private:
    int m_ChopCount = 0;
    const int m_ChopsRequired = 3;
    float m_ChopCooldown = 0.0f;

    float m_VisualJumpY = 0.0f;
    const float m_BaseYOffset = 0.7f;

    float m_AutoChopTimer = 0.0f;
    const float m_AutoChopInterval = 0.8f;

    Entity m_CursorKnife = { std::numeric_limits<std::size_t>::max(), 0 };

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

    IngredientType GetChoppedType(IngredientType rawType)
    {
        switch (rawType) {
        case IngredientType::Tomato: return IngredientType::ChoppedTomato;
        case IngredientType::Baguette: return IngredientType::CutBaguette;
        case IngredientType::Cheese: return IngredientType::ChoppedCheese;
        case IngredientType::Ham: return IngredientType::ChoppedHam;
        case IngredientType::Mozzarella: return IngredientType::ChoppedMozzarella;
        default: return IngredientType::None;
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
    void OnCreate() override
    {
        // Pozwalamy klasie bazowej podpi¹æ swoje eventy fizyczne...
        MachineScript::OnCreate();

        // ... a nastêpnie od razu je usuwamy dla tego konkretnego obiektu! 
        // Deska u¿ywa w³asnego "radaru" matematycznego, wiêc nie chcemy, aby wielkie kolidery sera i szynki same wywo³ywa³y transfer.
        GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityClickedEvent>(m_ClickSubId);
        GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityClickedEvent>(m_FoodClickSubId);
        GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityHoveredEvent>(m_HoverSubId);
    }

    void OnDestroy() override
    {
        if (m_CursorKnife.id != std::numeric_limits<std::size_t>::max())
        {
            GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_CursorKnife });
        }

        // Zerujemy ID eventów, ¿eby klasa bazowa (MachineScript) nie wyrzuci³a b³êdu próbuj¹c usun¹æ coœ, co zrobiliœmy w OnCreate
        m_ClickSubId = 0;
        m_FoodClickSubId = 0;
        m_HoverSubId = 0;

        MachineScript::OnDestroy();
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

        glm::vec3 mousePos = GetMouseWorldPosition();
        glm::vec2 mouse2D = { mousePos.x, mousePos.z };
        glm::vec2 board2D = { tf->GetPosition().x, tf->GetPosition().z };

        bool isHovering = (glm::distance(mouse2D, board2D) < 2.0f);

        bool shouldShowKnife = isHovering && !m_IsAutomated && !m_IsReady && !m_Ingredients.empty() && !GlobalIsMachineHeld;
        if (shouldShowKnife)
        {
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
                glm::vec3 knifePos = mousePos;
                knifePos.y = tf->GetPosition().y + 1.2f + m_VisualJumpY;
                knifeTf->SetPosition(knifePos);
            }
        }
        else
        {
            if (m_CursorKnife.id != std::numeric_limits<std::size_t>::max())
            {
                GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_CursorKnife });
                m_CursorKnife = { std::numeric_limits<std::size_t>::max(), 0 };
            }
        }

        if (isHovering && m_IsReady && !GlobalIsMachineHeld)
        {
            Entity closestPlate = GetClosestAvailablePlate();
            if (closestPlate.id != m_LastHighlightedPlate.id)
            {
                ClearHighlight();
                if (closestPlate.id != std::numeric_limits<std::size_t>::max())
                    SetPlateHighlight(closestPlate, true);
                m_LastHighlightedPlate = closestPlate;
            }
        }
        else if (!isHovering && m_LastHighlightedPlate.id != std::numeric_limits<std::size_t>::max())
        {
            ClearHighlight();
        }

        if (Input::IsMouseButtonJustPressed(0) && isHovering && !GlobalIsHoveringUI)
        {
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
                    TryTransferToPlate();
                }
                else if (!m_Ingredients.empty() && m_ChopCooldown <= 0.0f)
                {
                    PerformChop();
                }
            }
        }
    }

    virtual void HandleClick() override {}

    bool AddIngredient(IngredientType type) override
    {
        if (m_IsReady || !m_Ingredients.empty()) return false;

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
    void TryTransferToPlate() override
    {
        // ZABEZPIECZENIE: Upewniamy siê, ¿e minê³o chocia¿ u³amek sekundy od klikniêcia "Ciach!" nr 3.
        // Gwarantuje to, ¿e deska w ogóle nie spróbuje wydaæ dania w momencie dokoñczenia krojenia.
        if (m_ChopCooldown > 0.0f) return;

        Entity targetPlate = m_LastHighlightedPlate;

        if (targetPlate.id == std::numeric_limits<std::size_t>::max() && m_IsAutomated)
            targetPlate = GetClosestAvailablePlate();

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
                    spdlog::info("Sk³adnik z deski przeniesiony na talerz!");
                    ClearHighlight();
                    ResetMachineState();
                }
                else
                {
                    spdlog::warn("Talerz jest pe³ny lub nie mo¿e przyj¹æ sk³adnika!");
                }
            }
        }
        else
        {
            spdlog::warn("Brak podœwietlonego talerza - najedŸ na danie przed klikniêciem!");
        }
    }

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
            m_SpawnedFood = SpawnMachineFood(visualType, currentModelPath, "Na_Desce");
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
    }
};