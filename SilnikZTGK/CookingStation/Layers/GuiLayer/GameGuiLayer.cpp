#include "GameGuiLayer.h"
#include "EditorGuiLayer.h"
#include "Utils/Gui.h"
#include "Utils/Renderer2D.h"
#include "Utils/GuiUtils.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Events/EditorEvents.h" 
#include "CookingStation/Core/Application.h"
#include "CookingStation/json.hpp"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include <algorithm> 
#include "CookingStation/Scripts/DragAndDropScript.h"
#include "CookingStation/Scripts/Quests/DeliveryBoothScript.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include "CookingStation/Core/GameProgress.h"
#include "CookingStation/Core/VFS/VFS.h"
#include "CookingStation/Scene/PrefabSerializer.h"
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include "CookingStation/Events/GameEvents.h" 
#include <spdlog/spdlog.h>

bool GameGuiLayer::s_NeedsQuestReload = false;

void GameGuiLayer::OnAttach()
{
    m_ActiveScene = SceneManager::GetActiveScene();
    m_IsActive = true;

    if (!m_ActiveScene)
    {
        spdlog::error("GameGuiLayer: Nie znaleziono aktywnej sceny w OnAttach!");
        return;
    }

    // Inicjalizacja pod-panelu
    m_PausePanel = std::make_unique<PauseMenuPanel>();

#ifdef CS_DISTRIBUTION
    Gui::Init("assets://fonts/ARIAL.TTF", 32);
#endif

    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = (float)windowSize.first;
    m_ViewportHeight = (float)windowSize.second;

    // --- Ładowanie tekstur ---
    m_CornerIcon = AssetManager::GetTexture("assets://UI/bottomCornerClouds.png");
    m_TomatoIcon = AssetManager::GetTexture("assets://UI/tomato.png");
    m_CheeseIcon = AssetManager::GetTexture("assets://UI/Cheese.png");
    m_HamIcon = AssetManager::GetTexture("assets://UI/ham.png");
    m_BookCloudIcon = AssetManager::GetTexture("assets://UI/bookCloud.png");
    m_BookIcon = AssetManager::GetTexture("assets://UI/book.png");
    m_BookStarsIcon = AssetManager::GetTexture("assets://UI/bookStars.png");
    m_BookInsideIcon = AssetManager::GetTexture("assets://UI/bookInside.png");
    m_BookXIcon = AssetManager::GetTexture("assets://UI/bookX.png");
    m_TomatoSoupIcon = AssetManager::GetTexture("assets://UI/tomatoSoup.png");
    m_CoinIcon = AssetManager::GetTexture("assets://UI/coin.png");
    m_PotIcon = AssetManager::GetTexture("assets://UI/pot.png");
    m_MilkIcon = AssetManager::GetTexture("assets://UI/pot.png");
    m_FlourIcon = AssetManager::GetTexture("assets://UI/Flour.png");
    m_OvenIcon = AssetManager::GetTexture("assets://UI/oven.png");
    m_MixerIcon = AssetManager::GetTexture("assets://UI/pot.png");
    m_SandwichIcon = AssetManager::GetTexture("assets://UI/sandwich.png");
    m_CupcakeIcon = AssetManager::GetTexture("assets://UI/cupcake.png");
    m_CroissantIcon = AssetManager::GetTexture("assets://UI/croissant.png");

    m_IngredientsCarousel.Init(true);
    m_MachinesCarousel.Init(false);

    // ======================================================================================
    // Główna subskrypcja na zdarzenie STARTU GRY.
    // Wywoływane przez MainMenuLayer po załadowaniu nowej sceny!
    // ======================================================================================
    m_GameStartedSubId = Application::Get().GetEventBus().Subscribe<GameStartedEvent>(
        [this](const GameStartedEvent&) {

            // 1. Odpinamy stare subskrypcje ze starej (właśnie zniszczonej/ukrytej) sceny
            if (m_ActiveScene) {
                auto& oldBus = m_ActiveScene->GetWorld().GetEventBus();
                if (m_InventorySubId != 0) oldBus.Unsubscribe<InventoryChangedEvent>(m_InventorySubId);
                if (m_MoneySubId != 0) oldBus.Unsubscribe<MoneyChangedEvent>(m_MoneySubId);
            }

            // 2. Pobieramy NOWĄ, świeżo załadowaną scenę gry
            m_ActiveScene = SceneManager::GetActiveScene();

            // 3. Wymuszamy odświeżenie wymiarów GUI na ułamek sekundy przed jego pokazaniem
            // Zabezpiecza to przed błędami wyrenderowania, jeśli gracz zmieniał rozdzielczość w Menu.
            auto windowSize = Input::GetWindowSize();
            m_ViewportWidth = (float)windowSize.first;
            m_ViewportHeight = (float)windowSize.second;

            // 4. Zapinamy nasłuchiwanie zdarzeń na nowym EventBusie nowej sceny
            if (m_ActiveScene) {
                auto& newBus = m_ActiveScene->GetWorld().GetEventBus();

                // Nasłuchiwanie na zmianę ekwipunku
                m_InventorySubId = newBus.Subscribe<InventoryChangedEvent>(
                    [this](const InventoryChangedEvent& e) {
                        if (!m_IsActive) return;
                        std::string key;
                        switch (e.Type) {
                        case IngredientType::Tomato: key = "Tomato"; m_CurrentTomatoes = e.NewAmount; break;
                        case IngredientType::Cheese: key = "Cheese"; break;
                        case IngredientType::Ham:    key = "Ham";    break;
                        case IngredientType::Milk:   key = "Milk";   break;
                        case IngredientType::Flour:  key = "Flour";  break;
                        default: break;
                        }
                        if (!key.empty()) m_IngredientCounts[key] = e.NewAmount;
                    }
                );

                // Nasłuchiwanie na zmianę ilości pieniędzy
                m_MoneySubId = newBus.Subscribe<MoneyChangedEvent>(
                    [this](const MoneyChangedEvent& e) {
                        if (!m_IsActive) return;
                        m_CurrentMoney = e.NewAmount;
                        m_LastMoney = e.NewAmount;
                        m_MoneyStr = std::to_string(e.NewAmount);
                    }
                );
            }

            // 5. Włączamy widoczność GUI gry (zdejmujemy kurtynę!)
            SetVisible(true);
        }
    );
}

void GameGuiLayer::OnDetach()
{
    m_IsActive = false;

    if (m_ActiveScene) {
        auto& bus = m_ActiveScene->GetWorld().GetEventBus();
        if (m_InventorySubId != 0) {
            bus.Unsubscribe<InventoryChangedEvent>(m_InventorySubId);
            m_InventorySubId = 0;
        }
        if (m_MoneySubId != 0) {
            bus.Unsubscribe<MoneyChangedEvent>(m_MoneySubId);
            m_MoneySubId = 0;
        }
    }

    if (m_GameStartedSubId != 0) {
        Application::Get().GetEventBus().Unsubscribe<GameStartedEvent>(m_GameStartedSubId);
        m_GameStartedSubId = 0;
    }
}

bool GameGuiLayer::DrawBubblyImage(const std::string& id, const std::shared_ptr<Texture>& icon, glm::vec2 basePos, glm::vec2 baseSize, float dt, float hoverScale, bool darkenOnHover, float hitRadiusMultiplier, glm::vec4 tintColor, bool* outIsHovered)
{
    if (!icon) return false;
    auto& state = m_BubblyStates[id];
    glm::vec2 mousePos = Gui::GetMappedMousePos();
    float animSpeed = 15.0f;
    glm::vec2 center = { basePos.x + baseSize.x * 0.5f, basePos.y + baseSize.y * 0.5f };
    float hitRadius = std::min(baseSize.x, baseSize.y) * hitRadiusMultiplier;
    float distX = mousePos.x - center.x;
    float distY = mousePos.y - center.y;
    bool isHovered = (distX * distX + distY * distY) <= (hitRadius * hitRadius);

    if (outIsHovered != nullptr) *outIsHovered = isHovered;
    if (isHovered) MachineScript::GlobalIsHoveringUI = true;

    float targetScale = isHovered ? hoverScale : 1.0f;
    glm::vec4 targetColor = (isHovered && darkenOnHover) ? tintColor * glm::vec4(0.8f, 0.8f, 0.8f, 1.0f) : tintColor;

    state.scale += (targetScale - state.scale) * dt * animSpeed;
    state.color.r += (targetColor.r - state.color.r) * dt * animSpeed;
    state.color.g += (targetColor.g - state.color.g) * dt * animSpeed;
    state.color.b += (targetColor.b - state.color.b) * dt * animSpeed;

    glm::vec2 size = baseSize * state.scale;
    glm::vec2 pos = { basePos.x + (baseSize.x * 0.5f) - (size.x * 0.5f), basePos.y + (baseSize.y * 0.5f) - (size.y * 0.5f) };

    if (id == "CloudRight") Renderer2D::DrawQuad(pos, size, icon, state.color, { 1.0f, 1.0f }, { 0.0f, 0.0f });
    else Renderer2D::DrawQuad(pos, size, icon, state.color, { 0.0f, 1.0f }, { 1.0f, 0.0f });

    return (Input::IsMouseButtonJustPressed(0) && isHovered);
}

bool GameGuiLayer::DrawIngredientIcon(const std::string& id, const std::shared_ptr<Texture>& icon, glm::vec2 basePos, glm::vec2 baseSize, float dt, float baseScale, int count, bool showCount)
{
    bool isHovered = false;
    bool isClicked = DrawBubblyImage(id, icon, basePos, baseSize, dt, 1.30f, true, 0.5f, { 1.0f, 1.0f, 1.0f, 1.0f }, &isHovered);
    if (showCount && isHovered) {
        DrawIngredientCountText(count, basePos, baseSize, baseScale);
    }
    return isClicked;
}

void GameGuiLayer::DrawIngredientCountText(int count, glm::vec2 basePos, glm::vec2 baseSize, float baseScale)
{
    std::string countText = "x" + std::to_string(count);
    float textScale = 1.2f * baseScale;
    glm::vec2 textPos = { basePos.x + (baseSize.x * 0.05f), basePos.y + (baseSize.y * 0.25f) };
    glm::vec2 shadowPos = { textPos.x + 3.0f, textPos.y + 3.0f };
    Gui::DrawGuiText(countText, shadowPos, textScale, { 0.1f, 0.1f, 0.1f, 0.9f });
    Gui::DrawGuiText(countText, textPos, textScale, { 1.0f, 0.95f, 0.9f, 1.0f });
}

void GameGuiLayer::DrawRecipeIcon(const std::string& recipeId, const std::shared_ptr<Texture>& texture, glm::vec2 relativePct, float targetHeight, glm::vec2 bookPos, glm::vec2 bookSize, float dt)
{
    if (!texture) return;
    glm::vec2 size = GuiUtils::CalculateAspectSize(texture, targetHeight);
    glm::vec2 pos = { bookPos.x + bookSize.x * relativePct.x, bookPos.y + bookSize.y * relativePct.y };
    bool isUnlocked = GameProgress::IsRecipeUnlocked(recipeId);
    glm::vec4 tint = isUnlocked ? glm::vec4(1.0f) : glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);
    DrawBubblyImage("Recipe_" + recipeId, texture, pos, size, dt, 1.15f, true, 0.5f, tint);
}

void GameGuiLayer::DrawQuestPanel(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, bool isPlayMode) {
    if (!DeliveryBoothScript::s_Instance || !DeliveryBoothScript::s_Instance->HasActiveQuest()) return;
    const auto* activeQuest = DeliveryBoothScript::s_Instance->GetActiveQuest();
    if (!activeQuest) return;

    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    if (!activeScene || !activeScene->GetCamera()) return;

    auto* transform = activeScene->GetWorld().GetComponent<TransformComponent>(DeliveryBoothScript::s_Instance->GetEntity());
    if (!transform) return;

    glm::vec3 boothGlobalPos = glm::vec3(transform->WorldMatrix[3][0], transform->WorldMatrix[3][1], transform->WorldMatrix[3][2]);
    boothGlobalPos.y += 2.5f;

    auto* camera = activeScene->GetCamera();
    glm::mat4 view = camera->GetViewMatrix();
    float currentAspect = gameWidth / (gameHeight > 0.0f ? gameHeight : 1.0f);
    float orthoSize = camera->OrthoSize;
    glm::mat4 proj3D = glm::ortho(-currentAspect * orthoSize, currentAspect * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
    glm::mat4 viewProjection3D = proj3D * view;

    glm::vec4 clipSpacePos = viewProjection3D * glm::vec4(boothGlobalPos, 1.0f);
    if (clipSpacePos.w == 0.0f) return;

    glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;
    float boothScreenX = gameX + (ndcSpacePos.x + 1.0f) * 0.5f * gameWidth;
    float boothScreenY = gameY + (1.0f - ndcSpacePos.y) * 0.5f * gameHeight;

    glm::vec2 mousePos = Gui::GetMappedMousePos();
    float hoverRadius = 90.0f * baseScale;
    float dx = mousePos.x - boothScreenX;
    float dy = mousePos.y - (boothScreenY + 60.0f * baseScale);
    if ((dx * dx + dy * dy) > (hoverRadius * hoverRadius)) return;

    MachineScript::GlobalIsHoveringUI = true;

    glm::vec2 cloudSize = { 340.0f * baseScale, 225.0f * baseScale };
    glm::vec2 cloudPos = { boothScreenX - cloudSize.x * 0.5f, boothScreenY - cloudSize.y };

    if (cloudPos.x < gameX + 10.0f) cloudPos.x = gameX + 10.0f;
    if (cloudPos.x + cloudSize.x > gameX + gameWidth - 10.0f) cloudPos.x = gameX + gameWidth - cloudSize.x;
    if (cloudPos.y < gameY + 10.0f) cloudPos.y = gameY + 10.0f;

    Gui::Panel(cloudPos, cloudSize, { 0.08f, 0.08f, 0.1f, 0.96f }, 20.0f * baseScale);

    float textX = cloudPos.x + 16.0f * baseScale;
    float currentY = cloudPos.y + 15.0f * baseScale;
    float spacing = 24.0f * baseScale;

    Gui::DrawGuiText("AKTYWNY EVENT PRODUKCYJNY AI", { textX, currentY }, 0.42f * baseScale, { 1.0f, 0.5f, 0.1f, 1.0f });
    currentY += spacing + 5.0f * baseScale;
    Gui::DrawGuiText(activeQuest->Title, { textX, currentY }, 0.62f * baseScale, { 1.0f, 0.85f, 0.2f, 1.0f });
    currentY += spacing + 8.0f * baseScale;

    GuiUtils::DrawWrappedGuiText(activeQuest->Description, { textX, currentY }, 0.60f * baseScale, { 0.9f, 0.9f, 0.9f, 1.0f }, spacing, 30);

    float footerY = cloudPos.y + cloudSize.y - 60.0f * baseScale;
    std::string goalStr = "Wymagane: " + activeQuest->DishId + " (" + std::to_string(activeQuest->PortionsDelivered) + " / " + std::to_string(activeQuest->PortionsRequired) + " szt.)";
    Gui::DrawGuiText(goalStr, { textX, footerY }, 0.60f * baseScale, { 0.3f, 1.0f, 0.4f, 1.0f });
    Gui::DrawGuiText("Nagroda: " + activeQuest->Reward, { textX, footerY + 24.0f * baseScale }, 0.42f * baseScale, { 0.3f, 0.8f, 1.0f, 1.0f });
}

void GameGuiLayer::DrawIngredientClouds(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt) {
    if (!m_CornerIcon) return;

    m_IngredientsCarousel.OnUpdate(dt);
    m_MachinesCarousel.OnUpdate(dt);

    glm::vec2 baseIconSize = GuiUtils::CalculateAspectSize(m_CornerIcon, gameHeight * 0.30f);
    glm::vec2 leftPosBase = { gameX, gameY + gameHeight - baseIconSize.y };
    glm::vec2 rightPosBase = { gameX + gameWidth - baseIconSize.x, gameY + gameHeight - baseIconSize.y };

    DrawBubblyImage("CloudLeft", m_CornerIcon, leftPosBase, baseIconSize, dt, 1.15f, false, 0.55f);
    DrawBubblyImage("CloudRight", m_CornerIcon, rightPosBase, baseIconSize, dt, 1.15f, false);

    float itemBaseH = baseIconSize.y * 0.3f;
    glm::vec2 arcRadius = { baseIconSize.x * 0.66f, baseIconSize.y * 0.64f };
    float paddingX = 30.0f * baseScale;
    float paddingY = 10.0f * baseScale;

    glm::vec2 leftCenter = { gameX + paddingX, gameY + gameHeight - paddingY };

    struct UIIngredient { std::string id; std::shared_ptr<Texture> tex; IngredientType type; std::string modelPath; };
    std::vector<UIIngredient> leftItems = {
        {"BtnTomato", m_TomatoIcon, IngredientType::Tomato, "assets://models/skladniki/pomidor/pomidor.gltf"},
        {"BtnTCheese", m_CheeseIcon, IngredientType::Cheese, "assets://models/skladniki/ser/ser.gltf"},
        {"BtnHam", m_HamIcon, IngredientType::Ham, "assets://models/skladniki/szynka/szynka.gltf"},
        {"BtnMilk", m_MilkIcon, IngredientType::Milk, "assets://models/skladniki/mleko/milk.gltf"},
        {"BtnFlour", m_FlourIcon, IngredientType::Flour, "assets://models/skladniki/maka/maka.gltf"}
    };

    for (int i = 0; i < leftItems.size(); i++) {
        glm::vec2 actualSize = GuiUtils::CalculateAspectSize(leftItems[i].tex, itemBaseH);
        glm::vec2 pos;
        if (m_IngredientsCarousel.GetItemTransform(i, leftCenter, arcRadius, actualSize, pos)) {
            int countToDraw = GameManagerScript::s_Instance ? GameManagerScript::s_Instance->GetIngredientCount(leftItems[i].type) : 0;
            if (DrawIngredientIcon(leftItems[i].id, leftItems[i].tex, pos, actualSize, dt, baseScale, countToDraw, true)) {
                DragAndDropScript::StartDrag(leftItems[i].type, leftItems[i].modelPath);
            }
        }
    }

    glm::vec2 rightCenter = { gameX + gameWidth - paddingX, gameY + gameHeight - paddingY };
    struct UIMachine { std::string id; std::shared_ptr<Texture> tex; std::string prefabPath; };
    std::vector<UIMachine> rightItems = {
        {"BtnPot", m_PotIcon, "assets://prefabs/pot.json"},
        {"BtnMixer", m_PotIcon, "assets://prefabs/mixer.json"},
        {"BtnOven", m_OvenIcon, "assets://prefabs/oven.json"},
        {"BtnPot4", m_PotIcon, "assets://prefabs/pot.json"}
    };

    for (int i = 0; i < rightItems.size(); i++) {
        glm::vec2 actualSize = GuiUtils::CalculateAspectSize(rightItems[i].tex, itemBaseH);
        glm::vec2 pos;
        if (m_MachinesCarousel.GetItemTransform(i, rightCenter, arcRadius, actualSize, pos)) {
            if (DrawIngredientIcon(rightItems[i].id, rightItems[i].tex, pos, actualSize, dt, baseScale, 0, false)) {
                Entity spawnedMachine = PrefabSerializer::Deserialize(
                    SceneManager::GetActiveScene().get(),
                    rightItems[i].prefabPath,
                    glm::vec3(0.0f)
                );
                DragAndDropScript::PickupSpawnedMachine(spawnedMachine);
            }
        }
    }
}

void GameGuiLayer::DrawRecipeBook(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt) {
    if (m_BookIcon) {
        glm::vec2 cloudSize = { 280.0f * baseScale, 280.0f * baseScale };
        glm::vec2 cloudPos = { gameX + 20.0f * baseScale, gameY + 20.0f * baseScale };
        glm::vec2 actualCloudSize = cloudSize * 1.3f;

        if (m_BookCloudIcon) DrawBubblyImage("BookCloud", m_BookCloudIcon, cloudPos, actualCloudSize, dt, 1.1f, false);

        if (!m_IsRecipeBookOpen) {
            glm::vec2 bookSize = cloudSize * 1.1f;
            glm::vec2 bookPos = { cloudPos.x + (actualCloudSize.x - bookSize.x) * 0.5f, cloudPos.y + (actualCloudSize.y - bookSize.y) * 0.5f };

            if (DrawBubblyImage("BookIcon", m_BookIcon, bookPos, bookSize, dt, 1.15f, true, 0.35f)) m_IsRecipeBookOpen = true;
            if (m_BookStarsIcon) DrawBubblyImage("BookStars", m_BookStarsIcon, cloudPos, actualCloudSize, dt, 1.15f, false);
        }
        else {
            glm::vec2 insideSize = GuiUtils::CalculateAspectSize(m_BookInsideIcon, gameHeight * 1.0f);
            float yOffset = 50.0f * baseScale;
            glm::vec2 insidePos = { gameX + (gameWidth - insideSize.x) * 0.5f, gameY + (gameHeight - insideSize.y) * 0.5f + yOffset };

            if (m_BookInsideIcon) DrawBubblyImage("BookInside", m_BookInsideIcon, insidePos, insideSize, dt, 1.0f, false);

            glm::vec2 xSize = { 60.0f * baseScale, 60.0f * baseScale };
            glm::vec2 xPos = { insidePos.x + insideSize.x - xSize.x * 2.6f, insidePos.y + xSize.y * 2.6f };

            if (m_BookXIcon) {
                if (DrawBubblyImage("BookX", m_BookXIcon, xPos, xSize, dt, 1.2f, true, 0.4f)) m_IsRecipeBookOpen = false;
            }

            float recipeH = 120.0f * baseScale;
            DrawRecipeIcon("TomatoSoup", m_TomatoSoupIcon, { 0.12f, 0.15f }, recipeH, insidePos, insideSize, dt);
            DrawRecipeIcon("TomatoSoup", m_SandwichIcon, { 0.35f, 0.15f }, recipeH, insidePos, insideSize, dt);
            DrawRecipeIcon("TomatoSoup", m_CroissantIcon, { 0.12f, 0.30f }, recipeH - 20.0f, insidePos + 10.0f, insideSize, dt);
            DrawRecipeIcon("TomatoSoup", m_CupcakeIcon, { 0.35f, 0.30f }, recipeH, insidePos, insideSize, dt);
        }
    }
}

void GameGuiLayer::DrawIconWithText(const std::string& text, const std::shared_ptr<Texture>& iconTex, const glm::vec2& textPos, float textScale, float baseScale, float dt)
{
    if (!iconTex) return;
    float coinH = 80.0f * baseScale;
    glm::vec2 coinSize = { coinH, coinH };
    float textHeight = Gui::MeasureTextHeight(text, textScale);
    float baselineOffset = 32.0f * 0.8f * textScale;
    float textCenterY = textPos.y + baselineOffset - (textHeight * 0.5f);
    glm::vec2 coinPos = { textPos.x - coinSize.x - 8.0f * baseScale, textCenterY - (coinSize.y * 0.5f) };

    DrawBubblyImage("CoinIcon", iconTex, coinPos, coinSize, dt, 1.05f, false);
    Gui::DrawGuiText(text, { textPos.x + 2.0f, textPos.y + 2.0f }, textScale, { 0.0f, 0.0f, 0.0f, 0.85f });
    Gui::DrawGuiText(text, textPos, textScale, { 1.0f, 0.95f, 0.3f, 1.0f });
}

void GameGuiLayer::OnUpdate(Timestep ts) {
#ifdef CS_DISTRIBUTION
    if (!m_IsVisible) return;
#endif
    MachineScript::GlobalIsHoveringUI = false;

    Gui::BeginFrame();
    Gui::UpdateDeltaTime(ts.GetSeconds());
    float dt = ts.GetSeconds();

    if (s_NeedsQuestReload) {
        ReloadQuests();
        s_NeedsQuestReload = false;
    }

    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    bool isPlayMode = (activeScene && activeScene->GetState() == SceneState::Play);

    // ======================================================================================
    // PANCERNA POPRAWKA: Automatyczna, dynamiczna synchronizacja EventBusa w Edytorze i Grze
    // ======================================================================================
    static Scene* lastSubscribedScene = nullptr;
    static bool lastWasPlayMode = false;

    if (isPlayMode) {
        // Jeśli zmienił się wskaźnik sceny lub właśnie kliknięto PLAY (przejście z Edit do Play)
        if (activeScene.get() != lastSubscribedScene || !lastWasPlayMode) {

            // 1. Zwalniamy stare subskrypcje, jeśli były do czegoś przypięte
            if (lastSubscribedScene) {
                auto& oldBus = lastSubscribedScene->GetWorld().GetEventBus();
                if (m_InventorySubId != 0) oldBus.Unsubscribe<InventoryChangedEvent>(m_InventorySubId);
                if (m_MoneySubId != 0) oldBus.Unsubscribe<MoneyChangedEvent>(m_MoneySubId);
            }

            // 2. Zapamiętujemy aktualną instancję sceny uruchomieniowej
            lastSubscribedScene = activeScene.get();
            m_ActiveScene = activeScene; // Synchronizacja składowej klasy dla spójności OnDetach()

            auto& newBus = activeScene->GetWorld().GetEventBus();

            // 3. Rejestrujemy subskrypcję ekwipunku dla działającej sceny
            m_InventorySubId = newBus.Subscribe<InventoryChangedEvent>(
                [this](const InventoryChangedEvent& e) {
                    if (!m_IsActive) return;
                    std::string key;
                    switch (e.Type) {
                    case IngredientType::Tomato: key = "Tomato"; m_CurrentTomatoes = e.NewAmount; break;
                    case IngredientType::Cheese: key = "Cheese"; break;
                    case IngredientType::Ham:    key = "Ham";    break;
                    case IngredientType::Milk:   key = "Milk";   break;
                    case IngredientType::Flour:  key = "Flour";  break;
                    default: break;
                    }
                    if (!key.empty()) m_IngredientCounts[key] = e.NewAmount;
                }
            );

            // 4. Rejestrujemy subskrypcję pieniędzy dla działającej sceny
            m_MoneySubId = newBus.Subscribe<MoneyChangedEvent>(
                [this](const MoneyChangedEvent& e) {
                    if (!m_IsActive) return;
                    m_CurrentMoney = e.NewAmount;
                    m_LastMoney = e.NewAmount;
                    m_MoneyStr = std::to_string(e.NewAmount);
                }
            );

            // 5. Wymuszamy pobranie początkowego stanu portfela z GameManagerScript w tej klatce
            m_LastMoney = -1;
        }
    }
    else {
        // Jeśli wyszliśmy z trybu Play (kliknięto STOP), resetujemy wskaźnik pomocniczy
        lastSubscribedScene = nullptr;
    }
    lastWasPlayMode = isPlayMode;
    // ======================================================================================

#ifdef CS_DISTRIBUTION
    float gameX = 0.0f;
    float gameY = 0.0f;
    float gameWidth = m_ViewportWidth;
    float gameHeight = m_ViewportHeight;
#else
    float gameX = 200.0f;
    float gameY = 30.0f;
    float gameWidth = m_ViewportWidth - 500.0f;
    float gameHeight = m_ViewportHeight - 230.0f;
#endif

    if (gameWidth <= 0.0f || gameHeight <= 0.0f) return;
    float baseScale = std::max(gameHeight / 1080.0f, 0.5f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);

    glEnable(GL_SCISSOR_TEST);
    int scissorY = (int)(m_ViewportHeight - (gameY + gameHeight));
    glScissor((int)gameX, scissorY, (int)gameWidth, (int)gameHeight);

    Renderer2D::BeginScene(uiProj);
    DrawQuestPanel(gameX, gameY, gameWidth, gameHeight, baseScale, isPlayMode);
    DrawIngredientClouds(gameX, gameY, gameWidth, gameHeight, baseScale, dt);
    DrawRecipeBook(gameX, gameY, gameWidth, gameHeight, baseScale, dt);

    if (m_CoinIcon) {
        if (m_LastMoney == -1 && GameManagerScript::s_Instance) {
            int money = GameManagerScript::s_Instance->GetMoney();
            m_CurrentMoney = money;
            m_LastMoney = money;
            m_MoneyStr = std::to_string(money);
        }
        float textScale = 2.0f * baseScale;
        float textWidth = Gui::MeasureTextWidth(m_MoneyStr, textScale);
        glm::vec2 textPos = { gameX + gameWidth * 0.97f - textWidth, gameY + gameHeight * 0.02f };
        DrawIconWithText(m_MoneyStr, m_CoinIcon, textPos, textScale, baseScale, dt);
    }
    Renderer2D::EndScene();
    glDisable(GL_SCISSOR_TEST);

    // --- PAUSE MENU --- Z użyciem nowego systemu paneli!
    if (m_PausePanel && m_PausePanel->IsPaused()) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        Renderer2D::BeginScene(uiProj);
        m_PausePanel->OnUpdate(dt);
        m_PausePanel->Draw(baseScale);
        Renderer2D::EndScene();

        glEnable(GL_DEPTH_TEST);
    }

    glEnable(GL_DEPTH_TEST);
}

void GameGuiLayer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);

    // 1. ZAWSZE nasłuchuj zmiany rozmiaru okna, nawet gdy GUI gry jest ukryte!
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
        m_ViewportWidth = (float)ev.GetWidth();
        m_ViewportHeight = (float)ev.GetHeight();
        Gui::SetScreenSize(m_ViewportWidth, m_ViewportHeight);
        return false; // Zwracamy false, żeby event poleciał do warstwy Menu i silnika
        });

#ifdef CS_DISTRIBUTION
    // 2. Jeśli jesteśmy w Main Menu, zablokuj CAŁĄ RESZTĘ (żeby np. nie klikać guzików gry pod spodem menu)
    if (!m_IsVisible) return;
#endif

    // 3. Przekazywanie eventów do Panelu Pauzy (łapie klawisz ESC)
    if (m_PausePanel) {
        m_PausePanel->OnEvent(e);
        if (e.Handled) return; // Zapobiega klikaniu w grę, gdy opcje są otwarte
    }

    // 4. Reszta eventów gry (myszka, scrolle)
    dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& ev) {
        return OnMouseButtonPressed(ev);
        });

    dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& ev) {
        m_IngredientsCarousel.OnMouseScrolled(ev, m_ViewportWidth, 8);
        m_MachinesCarousel.OnMouseScrolled(ev, m_ViewportWidth, 8);
        return false;
        });

    // 5. OBSŁUGA WCIŚNIĘCIA PRZYCISKU PLAY W EDYTORZE (tylko questy)
    dispatcher.Dispatch<ScenePlayEvent>([this](ScenePlayEvent& ev) {
        ReloadQuests();
        return false;
        });
}

bool GameGuiLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e) {
    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    if (!activeScene || activeScene->GetState() != SceneState::Play) return false;
    if (Gui::WantCaptureMouse()) return true;
    return false;
}

void GameGuiLayer::ReloadQuests() {
    m_CurrentQuests.clear();
    std::vector<uint8_t> fileData = VFS::ReadFile("assets://wygenerowane_quests.json");

    if (!fileData.empty()) {
        try {
            nlohmann::json data = nlohmann::json::parse(fileData);
            for (auto& q : data) {
                m_CurrentQuests.push_back({
                    q.value("title", "Brak tytulu"),
                    q.value("description", "Brak opisu"),
                    q.value("portions", 0),
                    q.value("reward", "Brak nagrody")
                    });
            }
            m_CurrentQuestIndex = 0;
            spdlog::info("GameUiLayer: Questy zaladowane responsywnie przez VFS.");
        }
        catch (...) {
            spdlog::error("GameUiLayer: Blad JSON podczas parsowania z VFS.");
        }
    }
}