#include "GameGuiLayer.h"
#include "EditorGuiLayer.h"
#include "Utils/Gui.h"
#include "Utils/Renderer2D.h"
#include "Utils/GuiUtils.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Events/EditorEvents.h" 
#include "CookingStation/Core/Application.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scripts/DragAndDropScript.h"
#include "CookingStation/Scripts/Quests/DeliveryBoothScript.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include "CookingStation/Core/GameProgress.h"
#include "CookingStation/Core/VFS/VFS.h"
#include "CookingStation/Scene/PrefabSerializer.h"
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include "CookingStation/Events/KeyEvent.h"
#include "CookingStation/Events/GameEvents.h" 
#include "CookingStation/json.hpp"
#include "CookingStation/Scripts/Delivery/PackageScript.h"
#include <spdlog/spdlog.h>
#include <algorithm> 

void GameGuiLayer::OnAttach()
{
    m_ActiveScene = SceneManager::GetActiveScene();
    m_IsActive = true;

    if (!m_ActiveScene)
    {
        spdlog::error("GameGuiLayer: Nie znaleziono aktywnej sceny w OnAttach!");
        return;
    }

    m_PausePanel = std::make_unique<PauseMenuPanel>();

#ifdef CS_DISTRIBUTION
    Gui::Init("assets://fonts/FrankfurterMediumRegular.ttf", 32);
#endif

    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = (float)windowSize.first;
    m_ViewportHeight = (float)windowSize.second;

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
    m_CoinCloudIcon = AssetManager::GetTexture("assets://UI/coinCloud.png");
    m_PotIcon = AssetManager::GetTexture("assets://UI/pot.png");
    m_MilkIcon = AssetManager::GetTexture("assets://UI/pot.png");
    m_FlourIcon = AssetManager::GetTexture("assets://UI/Flour.png");
    m_OvenIcon = AssetManager::GetTexture("assets://UI/oven.png");
    m_MixerIcon = AssetManager::GetTexture("assets://UI/pot.png");
    m_SandwichIcon = AssetManager::GetTexture("assets://UI/sandwich.png");
    m_CupcakeIcon = AssetManager::GetTexture("assets://UI/cupcake.png");
    m_CroissantIcon = AssetManager::GetTexture("assets://UI/croissant.png");
    m_QuestionMarkIcon = AssetManager::GetTexture("assets://UI/QuestionMark.png");
    m_CustomerOrderTex = AssetManager::GetTexture("assets://UI/customerOrder.png");
    m_HelperOrderTex = AssetManager::GetTexture("assets://UI/helperOrder.png");

    m_MachineEntries = {
    { "Garnek",    "assets://prefabs/pot_station.json",   m_PotIcon   },
    { "Deska",     "assets://prefabs/board_station.json", m_FlourIcon },
    { "Mikser",    "assets://prefabs/mixer.json",         m_MixerIcon },
    { "Piekarnik", "assets://prefabs/oven.json",          m_OvenIcon  },
    };

    m_IngredientsCarousel.Init(true);
    m_MachinesCarousel.Init(false);

    m_GameStartedSubId = Application::Get().GetEventBus().Subscribe<GameStartedEvent>(
        [this](const GameStartedEvent&) {

            if (m_ActiveScene) {
                auto& oldBus = m_ActiveScene->GetWorld().GetEventBus();
                if (m_InventorySubId != 0) oldBus.Unsubscribe<InventoryChangedEvent>(m_InventorySubId);
                if (m_MoneySubId != 0) oldBus.Unsubscribe<MoneyChangedEvent>(m_MoneySubId);
                if (m_OrderTakenSubId != 0) oldBus.Unsubscribe<OrderTakenEvent>(m_OrderTakenSubId);
            }

            m_ActiveScene = SceneManager::GetActiveScene();
            m_ActiveOrderTickets.clear();

            auto windowSize = Input::GetWindowSize();
            m_ViewportWidth = (float)windowSize.first;
            m_ViewportHeight = (float)windowSize.second;

            if (m_ActiveScene) {
                auto& newBus = m_ActiveScene->GetWorld().GetEventBus();

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

                m_MoneySubId = newBus.Subscribe<MoneyChangedEvent>(
                    [this](const MoneyChangedEvent& e) {
                        if (!m_IsActive) return;
                        m_CurrentMoney = e.NewAmount;
                        m_LastMoney = e.NewAmount;
                        m_MoneyStr = std::to_string(e.NewAmount);
                    }
                );

                m_OrderTakenSubId = newBus.Subscribe<OrderTakenEvent>(
                    [this](const OrderTakenEvent& e) {
                        if (!m_IsActive) return;

                        auto it = std::find_if(m_ActiveOrderTickets.begin(), m_ActiveOrderTickets.end(),
                            [&e](const Entity& ticketEnt) {
                                return ticketEnt.id == e.Customer.id;
                            });

                        if (it == m_ActiveOrderTickets.end()) {
                            m_ActiveOrderTickets.push_back(e.Customer);
                        }
                    }
                );
            }

            SetVisible(true);
        }
    );

    auto& appBus = Application::Get().GetEventBus();

    appBus.Subscribe<GamePausedEvent>([this](const GamePausedEvent&) {
        m_IsGamePaused = true;
        });

    appBus.Subscribe<GameResumedEvent>([this](const GameResumedEvent&) {
        m_IsGamePaused = false;
        });

    appBus.Subscribe<BuildModeToggledEvent>([this](const BuildModeToggledEvent& e) {
        m_IsBuildModeActive = e.IsActive;
        if (!e.IsActive) {
            DeactivateBuildMode();
        }
        });
}

void GameGuiLayer::OnDetach()
{
    m_IsActive = false;

    if (m_ActiveScene) {
        auto& bus = m_ActiveScene->GetWorld().GetEventBus();
        if (m_InventorySubId != 0) { bus.Unsubscribe<InventoryChangedEvent>(m_InventorySubId); m_InventorySubId = 0; }
        if (m_MoneySubId != 0) { bus.Unsubscribe<MoneyChangedEvent>(m_MoneySubId); m_MoneySubId = 0; }
        if (m_OrderTakenSubId != 0) { bus.Unsubscribe<OrderTakenEvent>(m_OrderTakenSubId); m_OrderTakenSubId = 0; }
    }

    if (m_GameStartedSubId != 0) {
        Application::Get().GetEventBus().Unsubscribe<GameStartedEvent>(m_GameStartedSubId);
        m_GameStartedSubId = 0;
    }

    auto& appBus = Application::Get().GetEventBus();

    if (m_GamePausedSubId != 0) {
        appBus.Unsubscribe<GamePausedEvent>(m_GamePausedSubId);
        m_GamePausedSubId = 0;
    }

    if (m_GameResumedSubId != 0) {
        appBus.Unsubscribe<GameResumedEvent>(m_GameResumedSubId);
        m_GameResumedSubId = 0;
    }

}

bool GameGuiLayer::DrawBubblyImage(const std::string& id, const std::shared_ptr<Texture>& icon, glm::vec2 basePos, glm::vec2 baseSize, float dt, float hoverScale, bool darkenOnHover, float hitRadiusMultiplier, glm::vec4 tintColor, bool* outIsHovered)
{
    if (!icon) return false;
    auto& state = m_BubblyStates[id];
    glm::vec2 mousePos = Gui::GetMappedMousePos();

    if (m_IsGamePaused || (m_IsRecipeBookOpen && id.find("Book") == std::string::npos && id.find("Recipe") == std::string::npos)) {
        mousePos = glm::vec2(-10000.0f, -10000.0f);
    }

    float animSpeed = 15.0f;
    glm::vec2 center = { basePos.x + baseSize.x * 0.5f, basePos.y + baseSize.y * 0.5f };
    float hitRadius = std::min(baseSize.x, baseSize.y) * hitRadiusMultiplier * state.scale;
    float distX = mousePos.x - center.x;
    float distY = mousePos.y - center.y;
    bool isHovered = (distX * distX + distY * distY) <= (hitRadius * hitRadius);

    if (outIsHovered != nullptr) *outIsHovered = isHovered;
    if (isHovered) Input::SetUICaptureMouse(true);;

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
    if (m_IsRecipeBookOpen || m_IsGamePaused) return;
    if (!GameManagerScript::s_Instance) return;

    QuestEventState state = GameManagerScript::s_Instance->GetQuestState();
    if (state != QuestEventState::WaitingForAccept && state != QuestEventState::QuestActive) return;

    Entity targetEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    float yOffset3D = 2.5f;

    // 1. Zale¿nie od stanu szukamy odpowiedniego obiektu (wyspa vs ma³a budka)
    if (state == QuestEventState::WaitingForAccept) {
        // Szukamy du¿ej wyspy
        auto* tags = m_ActiveScene->GetWorld().GetComponentVector<TagComponent>();
        if (tags) {
            for (size_t i = 0; i < tags->dense.size(); ++i) {
                if (tags->dense[i].Tag == "event_78") {
                    targetEntity = tags->reverse[i];
                    break;
                }
            }
        }
        yOffset3D = 2.5f; // Du¿a wyspa jest wysoka
    }
    else if (state == QuestEventState::QuestActive) {
        // Szukamy ma³ej budki po tym, ¿e ma przypiêty skrypt "DeliveryBoothScript"
        auto* scripts = m_ActiveScene->GetWorld().GetComponentVector<NativeScriptComponent>();
        if (scripts) {
            for (size_t i = 0; i < scripts->dense.size(); ++i) {
                for (auto& s : scripts->dense[i].Scripts) {
                    if (s.Name == "DeliveryBoothScript") {
                        targetEntity = scripts->reverse[i];
                        break;
                    }
                }
                if (targetEntity.id != std::numeric_limits<std::size_t>::max()) break;
            }
        }

        // Zabezpieczenie awaryjne
        if (targetEntity.id == std::numeric_limits<std::size_t>::max()) {
            auto* tags = m_ActiveScene->GetWorld().GetComponentVector<TagComponent>();
            if (tags) {
                for (size_t i = 0; i < tags->dense.size(); ++i) {
                    if (tags->dense[i].Tag == "naro¿nikPas") {
                        targetEntity = tags->reverse[i];
                        break;
                    }
                }
            }
        }
        yOffset3D = 0.8f; // Ma³a budka jest malutka, obni¿amy punkt detekcji!
    }

    if (targetEntity.id == std::numeric_limits<std::size_t>::max()) return;

    QuestData* activeQuest = GameManagerScript::s_Instance->GetCurrentQuest();
    if (!activeQuest) return;

    auto* transform = m_ActiveScene->GetWorld().GetComponent<TransformComponent>(targetEntity);
    if (!transform) return;

    glm::vec3 boothGlobalPos = transform->GetPosition();
    boothGlobalPos.y += yOffset3D;

    auto* camera = m_ActiveScene->GetCamera();
    glm::mat4 view = camera->GetViewMatrix();
    float currentAspect = gameWidth / (gameHeight > 0.0f ? gameHeight : 1.0f);
    float orthoSize = camera->OrthoSize;
    glm::mat4 proj3D = glm::ortho(-currentAspect * orthoSize, currentAspect * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
    glm::mat4 viewProjection3D = proj3D * view;

    glm::vec4 clipSpacePos = viewProjection3D * glm::vec4(boothGlobalPos, 1.0f);
    if (clipSpacePos.w <= 0.0f) return;

    glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;
    float boothScreenX = gameX + (ndcSpacePos.x + 1.0f) * 0.5f * gameWidth;
    float boothScreenY = gameY + (1.0f - ndcSpacePos.y) * 0.5f * gameHeight;

    glm::vec2 cloudSize = { 380.0f * baseScale, (state == QuestEventState::WaitingForAccept ? 260.0f : 200.0f) * baseScale };
    glm::vec2 cloudPos = { boothScreenX - cloudSize.x * 0.5f, boothScreenY - cloudSize.y };

    if (cloudPos.x < gameX + 10.0f) cloudPos.x = gameX + 10.0f;
    if (cloudPos.x + cloudSize.x > gameX + gameWidth - 10.0f) cloudPos.x = gameX + gameWidth - cloudSize.x;
    if (cloudPos.y < gameY + 10.0f) cloudPos.y = gameY + 10.0f;

    // 2. Obs³uga najechania myszk¹
    glm::vec2 mousePos = Gui::GetMappedMousePos();
    float hoverRadius = (state == QuestEventState::WaitingForAccept ? 150.0f : 100.0f) * baseScale;
    float dx = mousePos.x - boothScreenX;
    float dy = mousePos.y - (boothScreenY + 30.0f * baseScale);

    bool isHovering3D = ((dx * dx + dy * dy) <= (hoverRadius * hoverRadius));

    float margin = 30.0f * baseScale;
    bool isHoveringPanel = (mousePos.x >= cloudPos.x - margin && mousePos.x <= cloudPos.x + cloudSize.x + margin &&
        mousePos.y >= cloudPos.y - margin && mousePos.y <= cloudPos.y + cloudSize.y + margin);

    if (!isHovering3D && !isHoveringPanel) return;

    Input::SetUICaptureMouse(true);

    Gui::Panel(cloudPos, cloudSize, { 0.08f, 0.08f, 0.1f, 0.96f }, 20.0f * baseScale);

    float textX = cloudPos.x + 16.0f * baseScale;
    float currentY = cloudPos.y + 15.0f * baseScale;
    float spacing = 24.0f * baseScale;

    Gui::DrawGuiText("AKTYWNY EVENT PRODUKCYJNY AI", { textX, currentY }, 0.42f * baseScale, { 1.0f, 0.5f, 0.1f, 1.0f });
    currentY += spacing + 5.0f * baseScale;
    Gui::DrawGuiText(activeQuest->Title, { textX, currentY }, 0.62f * baseScale, { 1.0f, 0.85f, 0.2f, 1.0f });
    currentY += spacing + 8.0f * baseScale;

    GuiUtils::DrawWrappedGuiText(activeQuest->Description, { textX, currentY }, 0.60f * baseScale, { 0.9f, 0.9f, 0.9f, 1.0f }, spacing, 30);

    float footerY = cloudPos.y + cloudSize.y - (state == QuestEventState::WaitingForAccept ? 100.0f : 45.0f) * baseScale;

    std::string goalStr = "Wymagane: " + activeQuest->DishID + " (0 / " + std::to_string(activeQuest->Portions) + " szt.)";
    Gui::DrawGuiText(goalStr, { textX, footerY }, 0.60f * baseScale, { 0.3f, 1.0f, 0.4f, 1.0f });
    Gui::DrawGuiText("Nagroda: " + std::to_string(activeQuest->RewardCoins) + " monet", { textX, footerY + 24.0f * baseScale }, 0.42f * baseScale, { 0.3f, 0.8f, 1.0f, 1.0f });

    if (state == QuestEventState::WaitingForAccept) {
        float buttonWidth = 140.0f * baseScale;
        float buttonHeight = 35.0f * baseScale;
        float buttonY = cloudPos.y + cloudSize.y - 45.0f * baseScale;

        if (Gui::Button("Zaakceptuj", { cloudPos.x + 10.0f * baseScale, buttonY }, { buttonWidth, buttonHeight })) {
            GameManagerScript::s_Instance->AcceptQuest();
        }

        int skipsLeft = GameManagerScript::s_Instance->GetSkipsLeft();
        std::string skipText = "Pomin (" + std::to_string(skipsLeft) + ")";
        bool canSkip = skipsLeft > 0;

        if (Gui::Button(skipText, { cloudPos.x + cloudSize.x - buttonWidth - 10.0f * baseScale, buttonY }, { buttonWidth, buttonHeight })) {
            if (canSkip) GameManagerScript::s_Instance->SkipQuest();
        }
    }
}

void GameGuiLayer::DrawIngredientClouds(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt) {
    if (!m_CornerIcon) return;

    if (!m_IsRecipeBookOpen) {
        m_IngredientsCarousel.OnUpdate(dt);
        m_MachinesCarousel.OnUpdate(dt);
    }

    glm::vec2 baseIconSize = GuiUtils::CalculateAspectSize(m_CornerIcon, gameHeight * 0.30f);
    glm::vec2 leftPosBase = { gameX, gameY + gameHeight - baseIconSize.y };
    glm::vec2 rightPosBase = { gameX + gameWidth - baseIconSize.x, gameY + gameHeight - baseIconSize.y };

    float itemBaseH = baseIconSize.y * 0.3f;
    glm::vec2 arcRadius = { baseIconSize.x * 0.66f, baseIconSize.y * 0.64f };
    float paddingX = 30.0f * baseScale;
    float paddingY = 10.0f * baseScale;
}

void GameGuiLayer::DrawRecipeBook(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt) {
    if (m_BookIcon) {
        glm::vec2 cloudSize = { 210.0f * baseScale, 210.0f * baseScale };
        glm::vec2 cloudPos = { gameX + 10.0f * baseScale, gameY * baseScale };
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

    float textWidth = Gui::MeasureTextWidth(text, textScale);
    float textHeight = Gui::MeasureTextHeight(text, textScale);
    float baselineOffset = 32.0f * 0.8f * textScale;

    float textCenterY = textPos.y + baselineOffset - (textHeight * 0.5f);
    float spacing = 8.0f * baseScale;
    glm::vec2 coinPos = { textPos.x - coinSize.x - spacing, textCenterY - (coinSize.y * 0.5f) };

    float paddingX = 45.0f * baseScale;
    float paddingY = 30.0f * baseScale;

    float totalContentWidth = coinSize.x + spacing + textWidth;
    float totalContentHeight = std::max(coinSize.y, textHeight);

    glm::vec2 cloudSize = { totalContentWidth + (paddingX * 2.0f), totalContentHeight + (paddingY * 2.0f) };
    glm::vec2 cloudPos = {
            coinPos.x - paddingX,
            textCenterY - (cloudSize.y * 0.5f)
    };

    DrawBubblyImage("CloudIcon", m_CoinCloudIcon, cloudPos, cloudSize, dt, 1.05f, false);

    DrawBubblyImage("CoinIcon", iconTex, coinPos, coinSize, dt, 1.05f, false);

    float coinCenterY = coinPos.y + (coinSize.y * 0.5f);
    float textDrawY = coinCenterY - baselineOffset + (textHeight * 0.25f);

    glm::vec2 shadowPos = { std::floor(textPos.x + 3.0f), std::floor(textDrawY + 3.0f) };
    glm::vec2 finalPos = { std::floor(textPos.x),         std::floor(textDrawY) };

    Gui::DrawGuiText(text, shadowPos, textScale, { 0.0f, 0.0f, 0.0f, 0.6f });
    Gui::DrawGuiText(text, finalPos, textScale, { 1.0f, 0.95f, 0.3f, 1.0f });
}

void GameGuiLayer::DrawOrderTickets(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale)
{
    if (!m_ActiveScene) return;

    auto* tags = m_ActiveScene->GetWorld().GetComponentVector<TagComponent>();
    auto* scripts = m_ActiveScene->GetWorld().GetComponentVector<NativeScriptComponent>();
    if (!tags || !scripts) return;

    bool isPlaying = (m_ActiveScene->GetState() == SceneState::Play);

    for (auto it = m_ActiveOrderTickets.begin(); it != m_ActiveOrderTickets.end(); ) {
        auto* nsc = scripts->Get(*it);
        if (!nsc) {
            it = m_ActiveOrderTickets.erase(it);
            continue;
        }

        CustomerScript* custScript = nullptr;
        for (auto& s : nsc->Scripts) {
            if (s.Name == "CustomerScript" || s.Name == "HelperCustomerScript") {
                custScript = (CustomerScript*)s.Instance;
                break;
            }
        }

        if (isPlaying) {
            if (!custScript || custScript->IsServed || custScript->IsPendingDestroy) {
                it = m_ActiveOrderTickets.erase(it);
                continue;
            }
        }
        ++it;
    }

    float currentY = gameY + (15.0f * baseScale);
    float rightMargin = 20.0f * baseScale;

    for (size_t i = 0; i < m_ActiveOrderTickets.size(); ++i) {
        Entity custEntity = m_ActiveOrderTickets[i];
        auto* tagComp = tags->Get(custEntity);

        if (!tagComp) continue;

        bool isFirst = (i == 0);
        float ticketHeight = isFirst ? (220.0f * baseScale) : (140.0f * baseScale);

        std::shared_ptr<Texture> ticketTex = (tagComp->Tag == "HelperCustomer") ? m_HelperOrderTex : m_CustomerOrderTex;

        if (!ticketTex) ticketTex = m_BookCloudIcon;

        if (ticketTex) {
            glm::vec2 ticketSize = GuiUtils::CalculateAspectSize(ticketTex, ticketHeight);
            glm::vec2 ticketPos = { gameX + gameWidth - ticketSize.x - rightMargin, currentY };

            Renderer2D::DrawQuad(ticketPos, ticketSize, ticketTex, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });

            currentY += ticketHeight + (10.0f * baseScale);
        }
    }
}

void GameGuiLayer::OnUpdate(Timestep ts) {
#ifdef CS_DISTRIBUTION
    if (!m_IsVisible) return;
#endif

    Input::SetUICaptureMouse(false);

    if (m_IsRecipeBookOpen || m_IsGamePaused) {
        Input::SetUICaptureMouse(true);
    }

    Gui::BeginFrame();
    Gui::UpdateDeltaTime(ts.GetSeconds());
    float dt = ts.GetSeconds();

    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    bool isPlayMode = (activeScene && activeScene->GetState() == SceneState::Play);
    bool isGameActive = (activeScene && (activeScene->GetState() == SceneState::Play || activeScene->GetState() == SceneState::Pause));

    static Scene* lastSubscribedScene = nullptr;
    static bool lastWasGameActive = false;

    if (isGameActive) {
        if (activeScene.get() != lastSubscribedScene || !lastWasGameActive) {
            if (lastSubscribedScene) {
                auto& oldBus = lastSubscribedScene->GetWorld().GetEventBus();
                if (m_InventorySubId != 0) oldBus.Unsubscribe<InventoryChangedEvent>(m_InventorySubId);
                if (m_MoneySubId != 0) oldBus.Unsubscribe<MoneyChangedEvent>(m_MoneySubId);
                if (m_OrderTakenSubId != 0) oldBus.Unsubscribe<OrderTakenEvent>(m_OrderTakenSubId);
            }

            lastSubscribedScene = activeScene.get();
            m_ActiveScene = activeScene;

            auto& newBus = activeScene->GetWorld().GetEventBus();

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

            m_MoneySubId = newBus.Subscribe<MoneyChangedEvent>(
                [this](const MoneyChangedEvent& e) {
                    if (!m_IsActive) return;
                    m_CurrentMoney = e.NewAmount;
                    m_LastMoney = e.NewAmount;
                    m_MoneyStr = std::to_string(e.NewAmount);
                }
            );

            m_OrderTakenSubId = newBus.Subscribe<OrderTakenEvent>(
                [this](const OrderTakenEvent& e) {
                    if (!m_IsActive) return;

                    auto it = std::find_if(m_ActiveOrderTickets.begin(), m_ActiveOrderTickets.end(),
                        [&e](const Entity& ticketEnt) {
                            return ticketEnt.id == e.Customer.id;
                        });

                    if (it == m_ActiveOrderTickets.end()) {
                        m_ActiveOrderTickets.push_back(e.Customer);
                    }
                }
            );

            m_LastMoney = -1;
            m_ActiveOrderTickets.clear();
        }
    }
    else {
        lastSubscribedScene = nullptr;
    }
    lastWasGameActive = isGameActive;

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
    DrawCustomerOrders(gameX, gameY, gameWidth, gameHeight, baseScale);
    DrawOrderTickets(gameX, gameY, gameWidth, gameHeight, baseScale);

    DrawPackageHoverInfo(gameX, gameY, gameWidth, gameHeight, baseScale, dt);
    DrawCrateHoverInfo(gameX, gameY, gameWidth, gameHeight, baseScale, dt);

    DrawBuildModeButton(gameX, gameY, gameWidth, gameHeight, baseScale, dt);
    DrawBuildModePanel(gameX, gameY, gameWidth, gameHeight, baseScale, dt);

    if (m_CoinIcon) {
        if (m_LastMoney == -1 && GameManagerScript::s_Instance) {
            int money = GameManagerScript::s_Instance->GetMoney();
            m_CurrentMoney = money;
            m_LastMoney = money;
            m_MoneyStr = std::to_string(money);
        }
        float textScale = 2.0f * baseScale;
        float textWidth = Gui::MeasureTextWidth(m_MoneyStr, textScale);

        float coinH = 80.0f * baseScale;
        float totalWidth = coinH + (8.0f * baseScale) + textWidth;

        float startX = gameX + (gameWidth - totalWidth) * 0.5f;
        glm::vec2 textPos = { startX + coinH + (8.0f * baseScale), gameY + 55.0f * baseScale };

        DrawIconWithText(m_MoneyStr, m_CoinIcon, textPos, textScale, baseScale, dt);
    }

    if (m_ShowFPS)
    {
        static float fpsTimer = 0.0f;
        static int currentFps = 0;

        fpsTimer += dt;
        if (fpsTimer >= 0.25f) {
            if (dt > 0.0f) currentFps = static_cast<int>(1.0f / dt);
            fpsTimer = 0.0f;
        }

        std::string fpsText = "FPS: " + std::to_string(currentFps);
        float fpsTextScale = 1.0f * baseScale;
        glm::vec2 fpsPos = { gameX + 20.0f * baseScale, gameY + 15.0f * baseScale };

        Gui::DrawGuiText(fpsText, { fpsPos.x + 2.0f, fpsPos.y + 2.0f }, fpsTextScale, { 0.1f, 0.1f, 0.1f, 0.9f });
        Gui::DrawGuiText(fpsText, fpsPos, fpsTextScale, { 0.2f, 1.0f, 0.2f, 1.0f });
    }

    Renderer2D::EndScene();
    glDisable(GL_SCISSOR_TEST);

    if (m_IsBuildModeActive)
        UpdateBuildModePlacement();

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

    if (m_IsBuildModeActive && m_PausePanel && !m_PausePanel->IsPaused()) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        Renderer2D::BeginScene(uiProj);
        DrawBuildModeOverlay(baseScale);
        Renderer2D::EndScene();

        glEnable(GL_DEPTH_TEST);
    }

    glEnable(GL_DEPTH_TEST);
}

void GameGuiLayer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);

    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
        m_ViewportWidth = (float)ev.GetWidth();
        m_ViewportHeight = (float)ev.GetHeight();
        Gui::SetScreenSize(m_ViewportWidth, m_ViewportHeight);
        return false;
        });

#ifdef CS_DISTRIBUTION
    if (!m_IsVisible) return;
#endif

    if (m_IsRecipeBookOpen) {
        if (e.GetEventType() == EventType::MouseButtonPressed ||
            e.GetEventType() == EventType::MouseButtonReleased ||
            e.GetEventType() == EventType::MouseMoved ||
            e.GetEventType() == EventType::MouseScrolled)
        {
            e.Handled = true;
        }
    }

    if (m_IsBuildModeActive && e.GetEventType() == EventType::MouseScrolled) {
        e.Handled = true;
        return;
    }

    if (m_PausePanel) {
        m_PausePanel->OnEvent(e);
        if (e.Handled) return;
    }

    dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& ev) {
        return OnMouseButtonPressed(ev);
        });

    dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& ev) {
        m_IngredientsCarousel.OnMouseScrolled(ev, m_ViewportWidth, 8);
        m_MachinesCarousel.OnMouseScrolled(ev, m_ViewportWidth, 8);

        std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
        if (activeScene && activeScene->GetState() == SceneState::Play) {
            return true;
        }

        return false;
        });


    dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
        if (ev.GetKeyCode() == 292) {
            if (ev.GetRepeatCode() == 0) {
                m_ShowFPS = !m_ShowFPS;
            }
        }

        if (ev.GetKeyCode() == 258 && ev.GetRepeatCode() == 0) {
            if (m_IsBuildModeActive) DeactivateBuildMode();
            else                     ActivateBuildMode();
            return true;
        }

        return false;
        });
}

bool GameGuiLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e) {
    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    if (!activeScene || activeScene->GetState() != SceneState::Play) return false;
    if (Gui::WantCaptureMouse()) return true;
    return false;
}

void GameGuiLayer::DrawCustomerOrders(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale)
{
    if (!m_ActiveScene || !m_ActiveScene->GetCamera()) return;

    auto* tags = m_ActiveScene->GetWorld().GetComponentVector<TagComponent>();
    auto* scripts = m_ActiveScene->GetWorld().GetComponentVector<NativeScriptComponent>();
    auto* transforms = m_ActiveScene->GetWorld().GetComponentVector<TransformComponent>();
    if (!tags || !scripts || !transforms) return;

    auto* camera = m_ActiveScene->GetCamera();
    glm::mat4 view = camera->GetViewMatrix();
    float currentAspect = gameWidth / (gameHeight > 0.0f ? gameHeight : 1.0f);
    float orthoSize = camera->OrthoSize;
    glm::mat4 proj3D = glm::ortho(-currentAspect * orthoSize, currentAspect * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
    glm::mat4 viewProj = proj3D * view;

    for (size_t i = 0; i < tags->dense.size(); ++i) {
        std::string tag = tags->dense[i].Tag;
        if (tag == "NormalCustomer" || tag == "HelperCustomer") {
            Entity custEntity = tags->reverse[i];
            auto* nsc = scripts->Get(custEntity);
            auto* tf = transforms->Get(custEntity);
            if (!nsc || !tf) continue;

            CustomerScript* custScript = nullptr;
            for (auto& s : nsc->Scripts) {
                if (s.Name == "CustomerScript" || s.Name == "HelperCustomerScript") {
                    custScript = (CustomerScript*)s.Instance;
                    break;
                }
            }

            if (!custScript || custScript->IsServed) continue;

            glm::vec3 headPos = tf->GetPosition() + glm::vec3(0.0f, 3.0f, 0.0f);
            glm::vec4 clipSpace = viewProj * glm::vec4(headPos, 1.0f);
            if (clipSpace.w == 0.0f) continue;

            glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;
            float screenX = gameX + (ndc.x + 1.0f) * 0.5f * gameWidth;
            float screenY = gameY + (1.0f - ndc.y) * 0.5f * gameHeight;

            std::shared_ptr<Texture> iconToDraw = nullptr;
            if (!custScript->OrderTaken) {
                iconToDraw = m_QuestionMarkIcon;
            }

            if (iconToDraw) {
                glm::vec2 iconSize = GuiUtils::CalculateAspectSize(iconToDraw, 70.0f * baseScale);
                glm::vec2 iconPos = { screenX - iconSize.x * 0.5f, screenY - iconSize.y };
                Renderer2D::DrawQuad(iconPos, iconSize, iconToDraw, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
            }
        }
    }
}

void GameGuiLayer::DrawHoverCloudUI(const glm::vec2& screenPos, const std::shared_ptr<Texture>& icon, int amount, float baseScale)
{
    if (!icon) return;

    glm::vec2 cloudSize = { 150.0f * baseScale, 150.0f * baseScale };
    glm::vec2 cloudPos = { screenPos.x - cloudSize.x * 0.5f, screenPos.y - cloudSize.y };

    if (m_BookCloudIcon) {
        Renderer2D::DrawQuad(cloudPos, cloudSize, m_BookCloudIcon, { 1.0f, 1.0f, 1.0f, 0.95f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    }

    glm::vec2 iconSize = GuiUtils::CalculateAspectSize(icon, 55.0f * baseScale);
    glm::vec2 iconPos = { cloudPos.x + (cloudSize.x - iconSize.x) * 0.5f, cloudPos.y + (cloudSize.y - iconSize.y) * 0.5f - (10.0f * baseScale) };

    Renderer2D::DrawQuad(iconPos, iconSize, icon, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });

    std::string amountStr = "x" + std::to_string(amount);
    float textScale = 1.3f * baseScale;
    float textWidth = Gui::MeasureTextWidth(amountStr, textScale);

    glm::vec2 textPos = { cloudPos.x + (cloudSize.x - textWidth) * 0.5f, iconPos.y + iconSize.y - (6.0f * baseScale) };

    glm::vec4 textColor = (amount > 0) ? glm::vec4(0.118f, 0.737f, 0.451f, 1.0f) : glm::vec4(1.0f, 0.3f, 0.3f, 1.0f);

    Gui::DrawGuiText(amountStr, { textPos.x + 2.0f, textPos.y + 2.0f }, textScale, { 0.1f, 0.1f, 0.1f, 0.6f });
    Gui::DrawGuiText(amountStr, textPos, textScale, textColor);
}

void GameGuiLayer::DrawPackageHoverInfo(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt)
{
    if (!m_ActiveScene || !m_ActiveScene->GetCamera()) return;

    auto* tags = m_ActiveScene->GetWorld().GetComponentVector<TagComponent>();
    auto* transforms = m_ActiveScene->GetWorld().GetComponentVector<TransformComponent>();
    auto* scripts = m_ActiveScene->GetWorld().GetComponentVector<NativeScriptComponent>();

    if (!tags || !transforms || !scripts) return;

    auto* camera = m_ActiveScene->GetCamera();
    glm::mat4 view = camera->GetViewMatrix();
    float currentAspect = gameWidth / (gameHeight > 0.0f ? gameHeight : 1.0f);
    float orthoSize = camera->OrthoSize;
    glm::mat4 proj3D = glm::ortho(-currentAspect * orthoSize, currentAspect * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
    glm::mat4 viewProj = proj3D * view;

    glm::vec2 mousePos = Gui::GetMappedMousePos();
    float hoverRadiusSq = (45.0f * baseScale) * (45.0f * baseScale);

    for (size_t i = 0; i < tags->dense.size(); ++i) {

        if (tags->dense[i].Tag.find("Package") != std::string::npos) {

            Entity packageEnt = tags->reverse[i];
            auto* tf = transforms->Get(packageEnt);
            if (!tf) continue;

            glm::vec3 packagePos = tf->GetPosition() + glm::vec3(0.0f, 0.8f, 0.0f);
            glm::vec4 clipSpace = viewProj * glm::vec4(packagePos, 1.0f);
            if (clipSpace.w == 0.0f) continue;

            glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;
            float screenX = gameX + (ndc.x + 1.0f) * 0.5f * gameWidth;
            float screenY = gameY + (1.0f - ndc.y) * 0.5f * gameHeight;

            float dx = mousePos.x - screenX;
            float dy = mousePos.y - (screenY + 40.0f * baseScale);

            if ((dx * dx + dy * dy) > hoverRadiusSq) continue;

            auto* nsc = scripts->Get(packageEnt);
            if (!nsc) continue;

            PackageScript* packScript = nullptr;
            for (auto& s : nsc->Scripts) {
                if (s.Name == "PackageScript") {
                    packScript = (PackageScript*)s.Instance;
                    break;
                }
            }

            if (!packScript) continue;

            std::shared_ptr<Texture> iconToDraw = nullptr;
            switch (packScript->getType()) {
            case IngredientType::Tomato: iconToDraw = m_TomatoIcon; break;
            case IngredientType::Cheese: iconToDraw = m_CheeseIcon; break;
            case IngredientType::Ham:    iconToDraw = m_HamIcon;    break;
            case IngredientType::Milk:   iconToDraw = m_MilkIcon;   break;
            case IngredientType::Flour:  iconToDraw = m_FlourIcon;  break;
            default: iconToDraw = m_QuestionMarkIcon; break;
            }

            DrawHoverCloudUI({ screenX, screenY }, iconToDraw, packScript->getIngredientAmount(), baseScale);
        }
    }
}

void GameGuiLayer::DrawCrateHoverInfo(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt)
{
    if (!m_ActiveScene || !m_ActiveScene->GetCamera()) return;

    auto* transforms = m_ActiveScene->GetWorld().GetComponentVector<TransformComponent>();
    auto* scripts = m_ActiveScene->GetWorld().GetComponentVector<NativeScriptComponent>();

    if (!transforms || !scripts) return;

    auto* camera = m_ActiveScene->GetCamera();
    glm::mat4 view = camera->GetViewMatrix();
    float currentAspect = gameWidth / (gameHeight > 0.0f ? gameHeight : 1.0f);
    float orthoSize = camera->OrthoSize;
    glm::mat4 proj3D = glm::ortho(-currentAspect * orthoSize, currentAspect * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
    glm::mat4 viewProj = proj3D * view;

    glm::vec2 mousePos = Gui::GetMappedMousePos();
    float hoverRadiusSq = (55.0f * baseScale) * (55.0f * baseScale);

    for (size_t i = 0; i < scripts->dense.size(); ++i) {
        auto& nsc = scripts->dense[i];

        CrateScript* crateScript = nullptr;
        for (auto& s : nsc.Scripts) {
            if (s.Name == "CrateScript") {
                crateScript = (CrateScript*)s.Instance;
                break;
            }
        }

        if (!crateScript || crateScript->m_CrateIngredient == IngredientType::None) continue;

        Entity crateEnt = scripts->reverse[i];
        auto* tf = transforms->Get(crateEnt);
        if (!tf) continue;

        glm::vec3 cratePos = tf->GetPosition() + glm::vec3(0.0f, 1.2f, 0.0f);
        glm::vec4 clipSpace = viewProj * glm::vec4(cratePos, 1.0f);
        if (clipSpace.w == 0.0f) continue;

        glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;
        float screenX = gameX + (ndc.x + 1.0f) * 0.5f * gameWidth;
        float screenY = gameY + (1.0f - ndc.y) * 0.5f * gameHeight;

        float dx = mousePos.x - screenX;
        float dy = mousePos.y - (screenY + 40.0f * baseScale);

        if ((dx * dx + dy * dy) > hoverRadiusSq) continue;

        std::shared_ptr<Texture> iconToDraw = nullptr;
        switch (crateScript->m_CrateIngredient) {
        case IngredientType::Tomato: iconToDraw = m_TomatoIcon; break;
        case IngredientType::Cheese: iconToDraw = m_CheeseIcon; break;
        case IngredientType::Ham:    iconToDraw = m_HamIcon;    break;
        case IngredientType::Milk:   iconToDraw = m_MilkIcon;   break;
        case IngredientType::Flour:  iconToDraw = m_FlourIcon;  break;
        default: iconToDraw = m_QuestionMarkIcon; break;
        }

        int amount = GameManagerScript::s_Instance ? GameManagerScript::s_Instance->GetIngredientCount(crateScript->m_CrateIngredient) : 0;
        DrawHoverCloudUI({ screenX, screenY }, iconToDraw, amount, baseScale);
    }
}

void GameGuiLayer::ActivateBuildMode()
{
    if (m_IsBuildModeActive) return;
    m_IsBuildModeActive = true;
    m_HeldMachineIndex = -1;

    auto activeScene = SceneManager::GetActiveScene();
    if (activeScene) activeScene->SetState(SceneState::Pause);

    Application::Get().GetEventBus().Publish(GamePausedEvent{});
}

void GameGuiLayer::DeactivateBuildMode()
{
    spdlog::info("DeactivateBuildMode wywolane, m_IsBuildModeActive={}", m_IsBuildModeActive);
    if (!m_IsBuildModeActive) return;

    m_IsBuildModeActive = false;
    m_HeldMachineIndex = -1;

    auto activeScene = SceneManager::GetActiveScene();
    if (!m_PreviewGroup.empty() && activeScene) {
        for (auto& [ent, offset] : m_PreviewGroup) {
            activeScene->GetWorld().GetEventBus().Publish(
                EntityDestroyRequestEvent{ ent });
        }
        m_PreviewGroup.clear();
    }

    if (m_MovingMachineEntity.id != std::numeric_limits<std::size_t>::max() && activeScene) {
        auto* tc = activeScene->GetWorld().GetComponent<TransformComponent>(m_MovingMachineEntity);
        if (tc) tc->SetPosition(m_MovingMachineOriginalPos);
        m_MovingMachineEntity = { std::numeric_limits<std::size_t>::max(), 0 };
        m_MovingGroup.clear();
    }

    if (activeScene) activeScene->SetState(SceneState::Play);

    Application::Get().GetEventBus().Publish(GameResumedEvent{});
}


void GameGuiLayer::DrawBuildModeButton(float gameX, float gameY, float gameWidth,
    float gameHeight, float baseScale, float dt)
{
    if (m_IsGamePaused && !m_IsBuildModeActive) return;
    if (m_IsRecipeBookOpen) return;

    float bookCloudH = 210.0f * baseScale * 1.3f;
    glm::vec2 cloudSize = { 180.0f * baseScale, 70.0f * baseScale };
    glm::vec2 cloudPos = { gameX + 14.0f * baseScale,
                             gameY + bookCloudH + 8.0f * baseScale };

    glm::vec4 bgColor = m_IsBuildModeActive
        ? glm::vec4(0.20f, 0.45f, 0.90f, 0.95f)
        : glm::vec4(0.10f, 0.12f, 0.20f, 0.82f);
    Renderer2D::DrawQuad(cloudPos, cloudSize, bgColor, 18.0f * baseScale);

    std::string label = m_IsBuildModeActive ? "[ BUILD ]" : "Build Mode";
    float       textScale = 0.55f * baseScale;
    float       tw = Gui::MeasureTextWidth(label, textScale);
    float       th = Gui::MeasureTextHeight(label, textScale);
    glm::vec2   textPos = {
        cloudPos.x + (cloudSize.x - tw) * 0.5f,
        cloudPos.y + (cloudSize.y - th) * 0.5f - th * 0.15f
    };

    Gui::DrawGuiText(label, { textPos.x + 1.5f, textPos.y + 1.5f },
        textScale, { 0.0f, 0.0f, 0.0f, 0.55f });

    Gui::DrawGuiText(label, textPos, textScale,
        m_IsBuildModeActive
        ? glm::vec4(0.85f, 0.95f, 1.00f, 1.0f)
        : glm::vec4(0.70f, 0.80f, 1.00f, 1.0f));


    glm::vec2 mouse = Gui::GetMappedMousePos();
    bool inBounds = mouse.x >= cloudPos.x && mouse.x <= cloudPos.x + cloudSize.x
        && mouse.y >= cloudPos.y && mouse.y <= cloudPos.y + cloudSize.y;

    if (inBounds) {
        Input::SetUICaptureMouse(true);
        if (Input::IsMouseButtonJustPressed(0)) {
            if (m_IsBuildModeActive) DeactivateBuildMode();
            else                     ActivateBuildMode();
        }
    }
}


void GameGuiLayer::DrawBuildModePanel(float gameX, float gameY, float gameWidth,
    float gameHeight, float baseScale, float dt)
{
    float targetSlide = m_IsBuildModeActive ? 1.0f : 0.0f;
    m_BuildPanelSlideY += (targetSlide - m_BuildPanelSlideY) * std::min(dt * 14.0f, 1.0f);
    if (m_BuildPanelSlideY < 0.002f) m_BuildPanelSlideY = 0.0f;
    if (m_BuildPanelSlideY > 0.998f) m_BuildPanelSlideY = 1.0f;
    if (m_BuildPanelSlideY <= 0.0f) return;

    const float panelH = 130.0f * baseScale;
    float panelY = gameY + gameHeight - panelH * m_BuildPanelSlideY;

    Renderer2D::DrawQuad({ gameX, panelY }, { gameWidth, panelH },
        { 0.06f, 0.08f, 0.14f, 0.94f }, 0.0f);

    Renderer2D::DrawQuad({ gameX, panelY }, { gameWidth, 2.0f * baseScale },
        { 0.3f, 0.55f, 1.0f, 0.7f }, 0.0f);


    const float iconH = 80.0f * baseScale;
    const float iconW = iconH;
    const float spacing = 28.0f * baseScale;
    const int   count = (int)m_MachineEntries.size();
    const float totalW = count * iconW + (count - 1) * spacing;
    const float startX = gameX + (gameWidth - totalW) * 0.5f;
    const float iconY = panelY + (panelH - iconH) * 0.5f;

    glm::vec2 mouse = Gui::GetMappedMousePos();

    for (int i = 0; i < count; ++i) {
        const float ix = startX + i * (iconW + spacing);
        const float iy = iconY;

        const bool inIcon = mouse.x >= ix && mouse.x <= ix + iconW
            && mouse.y >= iy && mouse.y <= iy + iconH;
        const bool isHeld = (m_HeldMachineIndex == i);

        glm::vec4 bg = isHeld
            ? glm::vec4(0.30f, 0.60f, 1.00f, 0.50f)
            : (inIcon
                ? glm::vec4(1.00f, 1.00f, 1.00f, 0.18f)
                : glm::vec4(1.00f, 1.00f, 1.00f, 0.07f));
        Renderer2D::DrawQuad({ ix, iy }, { iconW, iconH }, bg, 8.0f * baseScale);

        auto& entry = m_MachineEntries[i];
        if (entry.Icon) {
            Renderer2D::DrawQuad({ ix, iy }, { iconW, iconH }, entry.Icon,
                { 1.0f, 1.0f, 1.0f, 1.0f },
                { 0.0f, 1.0f }, { 1.0f, 0.0f });
        }

        float ls = 0.42f * baseScale;
        float lw = Gui::MeasureTextWidth(entry.Label, ls);
        Gui::DrawGuiText(entry.Label,
            { ix + (iconW - lw) * 0.5f, iy + iconH + 5.0f * baseScale },
            ls, { 0.80f, 0.90f, 1.00f, 1.0f });

        if (inIcon && m_IsBuildModeActive) {
            Input::SetUICaptureMouse(true);
            if (Input::IsMouseButtonJustPressed(0) && m_HeldMachineIndex == -1) {
                m_HeldMachineIndex = i;
                m_JustSelectedFromPanel = true;
            }
        }
    }

    if (m_HeldMachineIndex >= 0 && m_BuildPanelSlideY > 0.95f) {
        const std::string hint = "LPM: postaw   PPM / Tab: anuluj";
        float hs = 0.44f * baseScale;
        float hw = Gui::MeasureTextWidth(hint, hs);
        Gui::DrawGuiText(hint,
            { gameX + (gameWidth - hw) * 0.5f, panelY - 26.0f * baseScale },
            hs, { 0.95f, 0.95f, 0.60f, 0.90f });
    }
}

void GameGuiLayer::UpdateBuildModePlacement()
{
    auto activeScene = SceneManager::GetActiveScene();
    if (!activeScene) return;

    if (Input::IsMouseButtonJustPressed(1)) {
        if (!m_PreviewGroup.empty()) {
            for (auto& [ent, offset] : m_PreviewGroup) {
                activeScene->GetWorld().GetEventBus().Publish(
                    EntityDestroyRequestEvent{ ent });
            }
            m_PreviewGroup.clear();
        }

        if (m_MovingMachineEntity.id != std::numeric_limits<std::size_t>::max()) {
            auto* tc = activeScene->GetWorld().GetComponent<TransformComponent>(m_MovingMachineEntity);
            if (tc) tc->SetPosition(m_MovingMachineOriginalPos);
            m_MovingMachineEntity = { std::numeric_limits<std::size_t>::max(), 0 };
            m_MovingGroup.clear();
        }
        m_HeldMachineIndex = -1;
        return;
    }

    auto mousePos = Input::GetMousePosition();
    auto windowSize = Input::GetWindowSize();

    float mouseX = mousePos.first;
    float mouseY = mousePos.second;
    float viewW = (float)windowSize.first;
    float viewH = (float)windowSize.second;

#ifndef CS_DISTRIBUTION
    mouseX -= 200.0f;
    mouseY -= 30.0f;
    viewW -= 500.0f;
    viewH -= 230.0f;
#endif

    auto* camera = activeScene->GetCamera();
    if (!camera) return;

    float aspect = viewW / (viewH > 0.0f ? viewH : 1.0f);
    float orthoSize = 10.0f * (camera->Zoom / 45.0f);
    glm::mat4 proj = glm::ortho(-aspect * orthoSize, aspect * orthoSize,
        -orthoSize, orthoSize, -100.0f, 100.0f);
    glm::mat4 view = camera->GetViewMatrix();

    Ray ray = Physics::CastRayFromMouse(mouseX, mouseY, viewW, viewH, proj, view);

    glm::vec3 snappedPos(0.0f);
    if (std::abs(ray.Direction.y) > 1e-6f) {
        float t = -ray.Origin.y / ray.Direction.y;
        if (t > 0.0f) {
            glm::vec3 hit = ray.Origin + t * ray.Direction;
            snappedPos = GridSystem::SnapToGrid(hit);
        }
    }

    if (m_HeldMachineIndex < 0 &&
        m_MovingMachineEntity.id != std::numeric_limits<std::size_t>::max())
    {
        auto& world = activeScene->GetWorld();

        auto* tc = world.GetComponent<TransformComponent>(m_MovingMachineEntity);
        if (tc) {
            glm::vec3 newPos = snappedPos;
            newPos.y = m_MovingMachineOriginalPos.y;
            tc->SetPosition(newPos);
        }

        for (auto& [groupEnt, offset] : m_MovingGroup) {
            auto* groupTc = world.GetComponent<TransformComponent>(groupEnt);
            if (groupTc) {
                glm::vec3 groupPos = snappedPos;
                groupPos.y = m_MovingMachineOriginalPos.y;
                groupPos += offset;
                groupPos.y = m_MovingMachineOriginalPos.y + offset.y;
                groupTc->SetPosition(groupPos);
            }
        }

        DrawBuildGrid(proj * view, camera->Position, snappedPos);

        auto rawMouse = Input::GetMousePosition();
        auto winSize = Input::GetWindowSize();
        float viewportBottomInWindow = (float)winSize.second - 30.0f;
        bool mouseOverPanel = (rawMouse.second >= viewportBottomInWindow - 130.0f);

        if (Input::IsMouseButtonJustPressed(0) && !mouseOverPanel && !m_JustSelectedFromPanel) {
            m_MovingMachineEntity = { std::numeric_limits<std::size_t>::max(), 0 };
            m_MovingGroup.clear();
        }

        m_JustSelectedFromPanel = false;
        return;
    }

    if (m_HeldMachineIndex < 0 &&
        m_MovingMachineEntity.id == std::numeric_limits<std::size_t>::max())
    {
        auto rawMouse = Input::GetMousePosition();
        bool mouseOverPanel = (rawMouse.second >=
            (float)Input::GetWindowSize().second - 130.0f);

        if (Input::IsMouseButtonJustPressed(0) && !mouseOverPanel)
        {
            Entity hit = Physics::GetHoveredEntity(
                Physics::CastRayFromMouse(mouseX, mouseY, viewW, viewH, proj, view),
                activeScene, true, true);
            spdlog::info("[BuildMode] GetHoveredEntity zwrocilo id={}", hit.id);

            if (Input::IsMouseButtonJustPressed(0) && !mouseOverPanel)
            {
                Entity hit = Physics::GetHoveredEntity(
                    Physics::CastRayFromMouse(mouseX, mouseY, viewW, viewH, proj, view),
                    activeScene, true, true);

                spdlog::info("[BuildMode] GetHoveredEntity zwrocilo id={}", hit.id);

                if (hit.id != std::numeric_limits<std::size_t>::max())
                {
                    auto* nsc = activeScene->GetWorld()
                        .GetComponent<NativeScriptComponent>(hit);
                    if (nsc)
                    {
                        bool isMachine = false;
                        for (auto& s : nsc->Scripts)
                        {
                            if (s.Name == "PotScript" ||
                                s.Name == "CuttingBoardScript" ||
                                s.Name == "MixerScript" ||
                                s.Name == "OvenScript")
                            {
                                isMachine = true;
                                break;
                            }
                        }

                        if (isMachine)
                        {
                            auto* tc = activeScene->GetWorld()
                                .GetComponent<TransformComponent>(hit);
                            if (tc)
                            {
                                m_MovingMachineOriginalPos = tc->GetPosition();
                                m_MovingMachineEntity = hit;

                                m_MovingGroup.clear();
                                glm::ivec2 machineCell = GridSystem::WorldToCell(tc->GetPosition());

                                auto* transforms = activeScene->GetWorld()
                                    .GetComponentVector<TransformComponent>();

                                if (transforms) {
                                    for (size_t idx = 0;
                                        idx < transforms->dense.size(); ++idx)
                                    {
                                        Entity candidate = transforms->reverse[idx];
                                        if (candidate.id == hit.id) continue;

                                        glm::vec3 candPos =
                                            transforms->dense[idx].GetPosition();
                                        glm::ivec2 candCell =
                                            GridSystem::WorldToCell(candPos);

                                        if (candCell == machineCell) {
                                            glm::vec3 offset = candPos - tc->GetPosition();
                                            m_MovingGroup.push_back({ candidate, offset });
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            return;
        }
    }

    if (m_HeldMachineIndex < 0 || m_HeldMachineIndex >= (int)m_MachineEntries.size()) return;
    auto& entry = m_MachineEntries[m_HeldMachineIndex];
    auto& world = activeScene->GetWorld();

    if (m_PreviewGroup.empty()) {
        std::vector<Entity> spawnedParts = PrefabSerializer::Deserialize(
            activeScene.get(), entry.PrefabPath, snappedPos);

        for (Entity ent : spawnedParts) {
            auto* tc = world.GetComponent<TransformComponent>(ent);
            glm::vec3 offset = tc ? (tc->GetPosition() - snappedPos) : glm::vec3(0.0f);

            m_PreviewGroup.push_back({ ent, offset });

            auto* tagComp = world.GetComponent<TagComponent>(ent);
            if (tagComp) tagComp->Tag = "__BuildPreview__";

            world.RemoveComponent<NativeScriptComponent>(ent);
            world.RemoveComponent<BoxColliderComponent>(ent);
        }
    }
    else {
        for (auto& [ent, offset] : m_PreviewGroup) {
            auto* tc = world.GetComponent<TransformComponent>(ent);
            if (tc) {
                glm::vec3 newPos = snappedPos + offset;
                newPos.y = snappedPos.y + offset.y;
                tc->SetPosition(newPos);
            }
        }
    }

    {
        glm::mat4 viewProj3D = proj * view;
        DrawBuildGrid(viewProj3D, camera->Position, snappedPos);
    }

    auto windowSize2 = Input::GetWindowSize();
    float panelH = 130.0f;
    auto rawMouse = Input::GetMousePosition();
    float rawY = rawMouse.second;
    float rawWinH = (float)windowSize2.second;
    bool mouseOverPanel = (rawY >= rawWinH - 130.0f);

    if (Input::IsMouseButtonJustPressed(0) && !mouseOverPanel && !m_JustSelectedFromPanel) {

        for (auto& [ent, _] : m_PreviewGroup) {
            world.GetEventBus().Publish(EntityDestroyRequestEvent{ ent });
        }
        m_PreviewGroup.clear();

        std::vector<Entity> placedEntities = PrefabSerializer::Deserialize(
            activeScene.get(), entry.PrefabPath, snappedPos);

        for (Entity ent : placedEntities) {
            auto* tagComp = world.GetComponent<TagComponent>(ent);
            if (tagComp) tagComp->Tag = entry.Label;
        }

        m_HeldMachineIndex = -1;
    }

    m_JustSelectedFromPanel = false;
}

void GameGuiLayer::DrawBuildModeOverlay(float baseScale)
{
    auto windowSize = Input::GetWindowSize();
    float W = (float)windowSize.first;
    float H = (float)windowSize.second;

    Renderer2D::DrawQuad({ 0.0f, 0.0f }, { W, H },
        { 0.05f, 0.05f, 0.05f, 0.15f }, 0.0f);

    auto pausedTextTex = AssetManager::GetTexture("assets://UI/pausedText.png");
    if (pausedTextTex && pausedTextTex->GetRendererID() != 0) {
        float aspect = (float)pausedTextTex->GetWidth() / (float)pausedTextTex->GetHeight();
        float targetW = W * 0.35f;
        glm::vec2 tSize = { targetW, targetW / aspect };
        glm::vec2 tPos = { (W - tSize.x) * 0.5f, H * 0.08f };
        Renderer2D::DrawQuad(tPos, tSize,
            pausedTextTex->GetRendererID(),
            { 1.0f, 1.0f, 1.0f, 1.0f },
            { 0.0f, 1.0f }, { 1.0f, 0.0f });
    }
}

void GameGuiLayer::DrawBuildGrid(const glm::mat4& viewProj3D,
    const glm::vec3& camPos,
    const glm::vec3& hoverPos)
{
    const float cell = GridSystem::CELL_SIZE;
    const float t = 0.06f;
    const float range = 30.0f;

    glm::vec4 lineColor = { 0.6f, 0.6f, 0.6f, 0.40f };
    glm::vec4 hoverColor = { 0.3f, 0.75f, 1.0f, 0.55f };

    int startX = (int)std::floor((camPos.x - range) / cell);
    int endX = (int)std::ceil((camPos.x + range) / cell);
    int startZ = (int)std::floor((camPos.z - range) / cell);
    int endZ = (int)std::ceil((camPos.z + range) / cell);

    float minX = startX * cell, maxX = endX * cell;
    float minZ = startZ * cell, maxZ = endZ * cell;
    float lenX = maxX - minX, lenZ = maxZ - minZ;
    float cX = (minX + maxX) * 0.5f;
    float cZ = (minZ + maxZ) * 0.5f;

    glm::ivec2 hCell = GridSystem::WorldToCell(hoverPos);
    glm::vec3  hCenter = { (hCell.x + 0.5f) * cell, 0.01f, (hCell.y + 0.5f) * cell };

    auto FlatQuad = [](const glm::vec3& center, float sx, float sz) -> glm::mat4 {
        return glm::translate(glm::mat4(1.0f), center)
            * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), { 1.f, 0.f, 0.f })
            * glm::scale(glm::mat4(1.0f), { sx, sz, 1.f })
            * glm::translate(glm::mat4(1.0f), { -0.5f, -0.5f, 0.f });
        };

    Renderer2D::EndScene();
    Renderer2D::BeginScene(viewProj3D);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Renderer2D::DrawQuad(FlatQuad(hCenter, cell, cell), hoverColor);

    for (int cz = startZ; cz <= endZ; ++cz)
        Renderer2D::DrawQuad(FlatQuad({ cX, 0.01f, cz * cell }, lenX, t), lineColor);

    for (int cx = startX; cx <= endX; ++cx)
        Renderer2D::DrawQuad(FlatQuad({ cx * cell, 0.01f, cZ }, t, lenZ), lineColor);

    Renderer2D::EndScene();
    glEnable(GL_DEPTH_TEST);
}