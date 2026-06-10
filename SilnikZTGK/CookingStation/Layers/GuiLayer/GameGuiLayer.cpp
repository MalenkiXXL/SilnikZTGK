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
            m_ActiveOrderTickets.clear(); // Reset karteczek na początek gry

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

    // --- SYSTEM BLOKOWANIA PRZEBIJANIA KLIKNIĘĆ (MASKOWANIE TŁA) ---
    // Jeśli książka jest otwarta, a przycisk NIE JEST częścią książki, to myszka udaje, że jej tam nie ma
    if (m_IsGamePaused || (m_IsRecipeBookOpen && id.find("Book") == std::string::npos && id.find("Recipe") == std::string::npos)) {
        mousePos = glm::vec2(-10000.0f, -10000.0f);
    }
    // --------------------------------------------------------------

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
    // --- SYSTEM BLOKOWANIA --- Jeśli książka jest otwarta, chowamy popup questów pod spodem!
    if (m_IsRecipeBookOpen || m_IsGamePaused) return;

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

    Input::SetUICaptureMouse(true);

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

    // --- SYSTEM BLOKOWANIA --- Aktualizujemy karuzele tylko jak książka jest wyłączona
    if (!m_IsRecipeBookOpen) {
        m_IngredientsCarousel.OnUpdate(dt);
        m_MachinesCarousel.OnUpdate(dt);
    }
    // -------------------------

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

    // Mierzymy tekst
    float textWidth = Gui::MeasureTextWidth(text, textScale);
    float textHeight = Gui::MeasureTextHeight(text, textScale);
    float baselineOffset = 32.0f * 0.8f * textScale;

    // Wyliczamy pozycje Y
    float textCenterY = textPos.y + baselineOffset - (textHeight * 0.5f);
    float spacing = 8.0f * baseScale;
    glm::vec2 coinPos = { textPos.x - coinSize.x - spacing, textCenterY - (coinSize.y * 0.5f) };

    // Definiujemy marginesy wokół zawartości
    float paddingX = 45.0f * baseScale;
    float paddingY = 30.0f * baseScale;

    // Szerokość to: szerokość monety + odstęp + szerokość tekstu
    float totalContentWidth = coinSize.x + spacing + textWidth;
    float totalContentHeight = std::max(coinSize.y, textHeight);

    // Rozmiar i pozycja chmury
    glm::vec2 cloudSize = { totalContentWidth + (paddingX * 2.0f), totalContentHeight + (paddingY * 2.0f) };
    glm::vec2 cloudPos = {
            coinPos.x - paddingX,
            textCenterY - (cloudSize.y * 0.5f)
    };

    // Rysujemy chmurę
    DrawBubblyImage("CloudIcon", m_CoinCloudIcon, cloudPos, cloudSize, dt, 1.05f, false);

    // Rysujemy monetę
    DrawBubblyImage("CoinIcon", iconTex, coinPos, coinSize, dt, 1.05f, false);

    float coinCenterY = coinPos.y + (coinSize.y * 0.5f);
    float textDrawY = coinCenterY - baselineOffset + (textHeight * 0.25f);

    glm::vec2 shadowPos = { std::floor(textPos.x + 3.0f), std::floor(textDrawY + 3.0f) };
    glm::vec2 finalPos  = { std::floor(textPos.x),         std::floor(textDrawY) };

    Gui::DrawGuiText(text, shadowPos, textScale, { 0.0f, 0.0f, 0.0f, 0.6f });
    Gui::DrawGuiText(text, finalPos, textScale, { 1.0f, 0.95f, 0.3f, 1.0f });
}

void GameGuiLayer::DrawOrderTickets(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale)
{
    if (!m_ActiveScene) return;

    auto* tags = m_ActiveScene->GetWorld().GetComponentVector<TagComponent>();
    auto* scripts = m_ActiveScene->GetWorld().GetComponentVector<NativeScriptComponent>();
    if (!tags || !scripts) return;

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

        if (!custScript || custScript->IsServed || custScript->IsPendingDestroy) {
            it = m_ActiveOrderTickets.erase(it);
            continue;
        }
        ++it;
    }

    float currentY = gameY + (15.0f * baseScale);
    float rightMargin = 20.0f * baseScale;

    for (size_t i = 0; i < m_ActiveOrderTickets.size(); ++i) {
        Entity custEntity = m_ActiveOrderTickets[i];
        auto* tagComp = tags->Get(custEntity);
        auto* nsc = scripts->Get(custEntity);
        if (!tagComp || !nsc) continue;

        CustomerScript* custScript = nullptr;
        for (auto& s : nsc->Scripts) {
            if (s.Name == "CustomerScript" || s.Name == "HelperCustomerScript") {
                custScript = (CustomerScript*)s.Instance;
                break;
            }
        }
        if (!custScript) continue;

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

    if (s_NeedsQuestReload) {
        ReloadQuests();
        s_NeedsQuestReload = false;
    }

    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    bool isPlayMode = (activeScene && activeScene->GetState() == SceneState::Play);

    static Scene* lastSubscribedScene = nullptr;
    static bool lastWasPlayMode = false;

    if (isPlayMode) {
        if (activeScene.get() != lastSubscribedScene || !lastWasPlayMode) {

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
    lastWasPlayMode = isPlayMode;

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

    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
        m_ViewportWidth = (float)ev.GetWidth();
        m_ViewportHeight = (float)ev.GetHeight();
        Gui::SetScreenSize(m_ViewportWidth, m_ViewportHeight);
        return false;
        });

#ifdef CS_DISTRIBUTION
    if (!m_IsVisible) return;
#endif

    // --- SYSTEM BLOKOWANIA PRZEBIJANIA KLIKNIĘĆ W ŚWIAT 3D ---
    if (m_IsRecipeBookOpen) {
        if (e.GetEventType() == EventType::MouseButtonPressed ||
            e.GetEventType() == EventType::MouseButtonReleased ||
            e.GetEventType() == EventType::MouseMoved ||
            e.GetEventType() == EventType::MouseScrolled)
        {
            e.Handled = true; // Zatrzymuje kliknięcie w przestrzeni 3D i dla okien poniżej!
        }
    }
    // ---------------------------------------------------------

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

    dispatcher.Dispatch<ScenePlayEvent>([this](ScenePlayEvent& ev) {
        ReloadQuests();
        return false;
        });

    dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
        if (ev.GetKeyCode() == 292) {
            if (ev.GetRepeatCode() == 0) {
                m_ShowFPS = !m_ShowFPS;
            }
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

        // 1. filtr po Tagu
        if (tags->dense[i].Tag.find("Package") != std::string::npos) {

            Entity packageEnt = tags->reverse[i];
            auto* tf = transforms->Get(packageEnt);
            if (!tf) continue;

            // 2. MATEMATYKA: Liczymy pozycję na ekranie
            glm::vec3 packagePos = tf->GetPosition() + glm::vec3(0.0f, 0.8f, 0.0f);
            glm::vec4 clipSpace = viewProj * glm::vec4(packagePos, 1.0f);
            if (clipSpace.w == 0.0f) continue;

            glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;
            float screenX = gameX + (ndc.x + 1.0f) * 0.5f * gameWidth;
            float screenY = gameY + (1.0f - ndc.y) * 0.5f * gameHeight;

            // 3. HOVER: Sprawdzamy czy kursor w ogóle jest w pobliżu
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

            Input::SetUICaptureMouse(true);

            // Mapowanie ikony
            std::shared_ptr<Texture> iconToDraw = nullptr;
            switch (packScript->getType()) {
                case IngredientType::Tomato: iconToDraw = m_TomatoIcon; break;
                case IngredientType::Cheese: iconToDraw = m_CheeseIcon; break;
                case IngredientType::Ham:    iconToDraw = m_HamIcon;    break;
                case IngredientType::Milk:   iconToDraw = m_MilkIcon;   break;
                case IngredientType::Flour:  iconToDraw = m_FlourIcon;  break;
                default: iconToDraw = m_QuestionMarkIcon; break;
            }

            // Rysowanie UI
            if (iconToDraw) {
                glm::vec2 cloudSize = { 120.0f * baseScale, 120.0f * baseScale };
                glm::vec2 cloudPos = { screenX - cloudSize.x * 0.5f, screenY - cloudSize.y };

                if (m_BookCloudIcon) {
                    Renderer2D::DrawQuad(cloudPos, cloudSize, m_BookCloudIcon, {1.0f, 1.0f, 1.0f, 0.95f}, {0.0f, 1.0f}, {1.0f, 0.0f});
                }

                glm::vec2 iconSize = GuiUtils::CalculateAspectSize(iconToDraw, 55.0f * baseScale);
                glm::vec2 iconPos = { cloudPos.x + (cloudSize.x - iconSize.x) * 0.5f, cloudPos.y + (cloudSize.y - iconSize.y) * 0.5f };

                Renderer2D::DrawQuad(iconPos, iconSize, iconToDraw, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
            }
        }
    }
}