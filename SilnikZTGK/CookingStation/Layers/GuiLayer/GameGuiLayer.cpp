#include "GameGuiLayer.h"
#include "EditorGuiLayer.h"
#include "Utils/Gui.h"
#include "Utils/Renderer2D.h"
#include "Utils/GuiUtils.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Core/Application.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include "CookingStation/Scripts/Quests/DeliveryBoothScript.h"
#include "CookingStation/Scripts/Delivery/PackageScript.h"
#include "CookingStation/Scripts/CustomerScript.h"
#include "CookingStation/Scripts/CrateScript.h"
#include "CookingStation/Scripts/TutorialManagerScript.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Layers/GuiLayer/Utils/AudioConfig.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath> 
#include "CookingStation/Scripts/HelperCustomerScript.h"

bool g_TriggerCloudTransition = false;

void GameGuiLayer::OnAttach()
{
    m_ActiveScene = SceneManager::GetActiveScene();
    m_IsActive = true;
    if (!m_ActiveScene) {
        spdlog::error("GameGuiLayer: Nie znaleziono aktywnej sceny!");
        return;
    }

    m_PausePanel = std::make_unique<PauseMenuPanel>();
    m_RecipeBookPanel.Init();

#ifdef CS_DISTRIBUTION
    Gui::Init("assets://fonts/FrankfurterMediumRegular.ttf", 32);
#endif

    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = (float)windowSize.first;
    m_ViewportHeight = (float)windowSize.second;

    m_SmileFaceIcon = AssetManager::GetTexture("assets://UI/smileFace.png");
    m_AngryFaceIcon = AssetManager::GetTexture("assets://UI/angryFace.png");
    m_CornerIcon = AssetManager::GetTexture("assets://UI/bottomCornerClouds.png");
    m_TomatoIcon = AssetManager::GetTexture("assets://UI/tomato.png");
    m_CheeseIcon = AssetManager::GetTexture("assets://UI/Cheese.png");
    m_HamIcon = AssetManager::GetTexture("assets://UI/ham.png");
    m_CoinIcon = AssetManager::GetTexture("assets://UI/coin.png");
    m_CoinCloudIcon = AssetManager::GetTexture("assets://UI/coinCloud.png");
    m_MilkIcon = AssetManager::GetTexture("assets://UI/milk.png");
    m_FlourIcon = AssetManager::GetTexture("assets://UI/Flour.png");
    m_QuestionMarkIcon = AssetManager::GetTexture("assets://UI/QuestionMark.png");
    m_CustomerOrderTex = AssetManager::GetTexture("assets://UI/customerOrder.png");
    m_HelperOrderTex = AssetManager::GetTexture("assets://UI/helperOrder.png");
    m_BookCloudIcon = AssetManager::GetTexture("assets://UI/bookCloud.png");
    m_QuestCloudTex = AssetManager::GetTexture("assets://UI/Events/ChooseEventCloud.png");
    m_AcceptButtonTex = AssetManager::GetTexture("assets://UI/Events/AcceptButton.png");
    m_SkipButtonTex = AssetManager::GetTexture("assets://UI/Events/SkipButton.png");
    m_SpeedUpIcon = AssetManager::GetTexture("assets://UI/speedUp.png");
    m_EventCloudTex = AssetManager::GetTexture("assets://UI/Events/EventCloud.png");
    m_EventRewardTex = AssetManager::GetTexture("assets://UI/Events/EventReward.png");
    m_AppleIcon = AssetManager::GetTexture("assets://UI/Apple.png");
    m_CoffeeBeanIcon = AssetManager::GetTexture("assets://UI/coffeBean.png");
    m_CoffeeMachineIcon = AssetManager::GetTexture("assets://UI/coffeeMachine.png");
    m_RaspberryIcon = AssetManager::GetTexture("assets://UI/raspberry.png");
    m_StrawberryIcon = AssetManager::GetTexture("assets://UI/strawberry.png");
    m_StarPowderIcon = AssetManager::GetTexture("assets://UI/StarPowder.png");
    m_MozzarellaIcon = AssetManager::GetTexture("assets://UI/mozarella.png");
    m_YawnIcon = AssetManager::GetTexture("assets://UI/yawn.png");
    m_PotatoIcon = AssetManager::GetTexture("assets://UI/potato.png");
    m_EggIcon = AssetManager::GetTexture("assets://UI/egg.png");
    m_BuildModePanel.Init(m_CoinIcon);
    m_IngredientsCarousel.Init(true);
    m_MachinesCarousel.Init(false);
    m_LevelCompletedPanel.Init();

    m_GameStartedSubId = Application::Get().GetEventBus().Subscribe<GameStartedEvent>(
        [this](const GameStartedEvent&) {
            if (m_ActiveScene) {
                auto& oldBus = m_ActiveScene->GetWorld().GetEventBus();
                if (m_InventorySubId != 0) { oldBus.Unsubscribe<InventoryChangedEvent>(m_InventorySubId); m_InventorySubId = 0; }
                if (m_MoneySubId != 0) { oldBus.Unsubscribe<MoneyChangedEvent>(m_MoneySubId);         m_MoneySubId = 0; }
                if (m_OrderTakenSubId != 0) { oldBus.Unsubscribe<OrderTakenEvent>(m_OrderTakenSubId);      m_OrderTakenSubId = 0; }
                if (m_MushroomAppearedSubId != 0) { oldBus.Unsubscribe<DeliveryMushroomAppearedEvent>(m_MushroomAppearedSubId); m_MushroomAppearedSubId = 0; }
                if (m_DeliveryCollectedSubId != 0) { oldBus.Unsubscribe<DeliveryCollectedEvent>(m_DeliveryCollectedSubId); m_DeliveryCollectedSubId = 0; }
            }

            m_HasShownOrderHint = false;
            m_RecipeBookPanel.Reset();

            m_ActiveScene = SceneManager::GetActiveScene();
            m_ActiveOrderTickets.clear();
            m_LastMoney = -1;

            m_FirstOrderTaken = false;
            m_IsLevelIntro = !GameManagerScript::s_IsTutorialMode;
            m_IsEventIntro = false;
            m_HasShownEventIntro = false;
            m_EventIntroTimer = 0.0f;

            m_BuildModePanel.ForceReset();

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
                        if (m_LastMoney != -1 && e.NewAmount > m_LastMoney) {
                            m_CoinAnimTimer = 1.0f;
                        }
                        m_CurrentMoney = e.NewAmount;
                        m_LastMoney = e.NewAmount;
                        m_MoneyStr = std::to_string(e.NewAmount);
                    }
                );

                m_OrderTakenSubId = newBus.Subscribe<OrderTakenEvent>(
                    [this](const OrderTakenEvent& e) {
                        if (!m_IsActive) return;

                        m_FirstOrderTaken = true;

                        auto it = std::find_if(m_ActiveOrderTickets.begin(), m_ActiveOrderTickets.end(),
                            [&e](const Entity& ticketEnt) {
                                return ticketEnt.id == e.Customer.id;
                            });
                        if (it == m_ActiveOrderTickets.end()) {
                            m_ActiveOrderTickets.push_back(e.Customer);
                        }
                    }
                );

                m_MushroomAppearedSubId = newBus.Subscribe<DeliveryMushroomAppearedEvent>(
                    [this](const DeliveryMushroomAppearedEvent& e) {
                        m_ShowMushroomBubble = true;
                        m_MushroomPos3D = e.WorldPosition;
                    }
                );

                m_DeliveryCollectedSubId = newBus.Subscribe<DeliveryCollectedEvent>(
                    [this](const DeliveryCollectedEvent&) {
                        m_ShowMushroomBubble = false;
                    }
                );

                m_LevelCompletedSubId = newBus.Subscribe<LevelCompletedEvent>(
                    [this](const LevelCompletedEvent& e) {
                        spdlog::warn("GUI: Odebrano LevelCompletedEvent! Money={} Stars={}", e.EarnedMoney, e.StarsEarned);
                        m_LevelCompletedPanel.Show(e.EarnedMoney, e.StarsEarned);
                        Input::SetUICaptureMouse(true);
                        Application::Get().GetEventBus().Publish(e);
                    }
                );

                m_MachineWarningSubId = m_ActiveScene->GetWorld().GetEventBus().Subscribe<MachineNeedsMoreIngredientsEvent>(
                        [this](const MachineNeedsMoreIngredientsEvent& e) {
                            m_MachineWarning.MachineEnt = e.Machine;
                            m_MachineWarning.Line1 = e.MessageLine1;
                            m_MachineWarning.Line2 = e.MessageLine2;
                            m_MachineWarning.Timer = e.Duration;
                        }
                );
            }

            SetVisible(true);
            m_IsGamePaused = false;
        }
    );

    auto& appBus = Application::Get().GetEventBus();
    m_GamePausedSubId = appBus.Subscribe<GamePausedEvent>([this](const GamePausedEvent&) { m_IsGamePaused = true; });
    m_GameResumedSubId = appBus.Subscribe<GameResumedEvent>([this](const GameResumedEvent&) { m_IsGamePaused = false; });

    m_BuildModeToggledSubId = appBus.Subscribe<BuildModeToggledEvent>([this](const BuildModeToggledEvent& e) {
        if (!e.IsActive) m_BuildModePanel.Deactivate();
        else             m_BuildModePanel.Activate();
        });

    m_ShowMainMenuSubId = appBus.Subscribe<ShowMainMenuEvent>([this](const ShowMainMenuEvent&) {
        if (m_BuildModePanel.IsActive()) {
            m_BuildModePanel.Deactivate();
        }
        });
}

void GameGuiLayer::OnDetach()
{
    m_IsActive = false;

    if (m_ActiveScene) {
        auto& bus = m_ActiveScene->GetWorld().GetEventBus();
        if (m_InventorySubId != 0) { bus.Unsubscribe<InventoryChangedEvent>(m_InventorySubId);  m_InventorySubId = 0; }
        if (m_MoneySubId != 0) { bus.Unsubscribe<MoneyChangedEvent>(m_MoneySubId);          m_MoneySubId = 0; }
        if (m_OrderTakenSubId != 0) { bus.Unsubscribe<OrderTakenEvent>(m_OrderTakenSubId);       m_OrderTakenSubId = 0; }
        if (m_MushroomAppearedSubId != 0) { bus.Unsubscribe<DeliveryMushroomAppearedEvent>(m_MushroomAppearedSubId); m_MushroomAppearedSubId = 0; }
        if (m_DeliveryCollectedSubId != 0) { bus.Unsubscribe<DeliveryCollectedEvent>(m_DeliveryCollectedSubId); m_DeliveryCollectedSubId = 0; }
        if (m_LevelCompletedSubId != 0) { bus.Unsubscribe<LevelCompletedEvent>(m_LevelCompletedSubId); m_LevelCompletedSubId = 0; }
        if (m_MachineWarningSubId != 0) { bus.Unsubscribe<MachineNeedsMoreIngredientsEvent>(m_MachineWarningSubId); m_MachineWarningSubId = 0; }
    }

    auto& appBus = Application::Get().GetEventBus();
    if (m_GameStartedSubId != 0) { appBus.Unsubscribe<GameStartedEvent>(m_GameStartedSubId);             m_GameStartedSubId = 0; }
    if (m_GamePausedSubId != 0) { appBus.Unsubscribe<GamePausedEvent>(m_GamePausedSubId);               m_GamePausedSubId = 0; }
    if (m_GameResumedSubId != 0) { appBus.Unsubscribe<GameResumedEvent>(m_GameResumedSubId);             m_GameResumedSubId = 0; }
    if (m_BuildModeToggledSubId != 0) { appBus.Unsubscribe<BuildModeToggledEvent>(m_BuildModeToggledSubId);   m_BuildModeToggledSubId = 0; }
    if (m_ShowMainMenuSubId != 0) { appBus.Unsubscribe<ShowMainMenuEvent>(m_ShowMainMenuSubId);           m_ShowMainMenuSubId = 0; }
}

void GameGuiLayer::DrawIconWithText(const std::string& text, const std::shared_ptr<Texture>& iconTex,
    const glm::vec2& textPos, float textScale, float baseScale, float dt)
{
    if (!iconTex) return;

    float shakeOffsetX = 0.0f;
    glm::vec4 textColor = { 1.0f, 0.95f, 0.3f, 1.0f };
    bool isWarning = false;

    if (GameManagerScript::s_Instance && GameManagerScript::s_Instance->m_MoneyWarningTimer > 0.0f) {
        isWarning = true;

        shakeOffsetX = std::sin(GameManagerScript::s_Instance->m_MoneyWarningTimer * 40.0f) * (6.0f * baseScale);
        textColor = { 0.75f, 0.35f, 0.35f, 1.0f };
    }

    float coinH = 80.0f * baseScale;
    glm::vec2 coinSize = { coinH, coinH };
    float textWidth = Gui::MeasureTextWidth(text, textScale);
    float textHeight = Gui::MeasureTextHeight(text, textScale);
    float baselineOffset = 32.0f * 0.8f * textScale;
    float textCenterY = textPos.y + baselineOffset - (textHeight * 0.5f);
    float spacing = 8.0f * baseScale;

    glm::vec2 coinPos = { textPos.x - coinSize.x - spacing + shakeOffsetX, textCenterY - (coinSize.y * 0.5f) };

    float paddingX = 45.0f * baseScale;
    float paddingY = 30.0f * baseScale;
    glm::vec2 cloudSize = { coinSize.x + spacing + textWidth + (paddingX * 2.0f), std::max(coinSize.y, textHeight) + (paddingY * 2.0f) };
    glm::vec2 cloudPos = { coinPos.x - paddingX, textCenterY - (cloudSize.y * 0.5f) };

    glm::vec2 mouse = Gui::GetMappedMousePos();
    bool isHoveringCloud = (mouse.x >= cloudPos.x && mouse.x <= cloudPos.x + cloudSize.x &&
        mouse.y >= cloudPos.y && mouse.y <= cloudPos.y + cloudSize.y);

    static bool s_wasMoneyCloudHovered = false;
    if (isHoveringCloud && !s_wasMoneyCloudHovered && !m_IsGamePaused) {
        AudioEngine::PlayLoopingSound("assets://sounds/hover_in_game.mp3", 0.15f, false);
        s_wasMoneyCloudHovered = true;
    }
    else if (!isHoveringCloud) {
        s_wasMoneyCloudHovered = false;
    }

    if (isWarning) {
        if (m_CoinCloudIcon)
            Renderer2D::DrawQuad(cloudPos, cloudSize, m_CoinCloudIcon, { 0.95f, 0.85f, 0.85f, 0.95f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
        Renderer2D::DrawQuad(coinPos, coinSize, iconTex, { 0.75f, 0.35f, 0.35f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    }
    else if (m_CoinAnimTimer > 0.0f) {
        float bounceOffset = std::sin(m_CoinAnimTimer * 3.14159f) * (40.0f * baseScale);

        glm::vec4 pastelGreen = glm::vec4(0.85f, 1.0f, 0.85f, 1.0f);
        glm::vec4 vibrantGreen = glm::vec4(0.40f, 1.0f, 0.50f, 1.0f);

        float colorMix = std::sin(m_CoinAnimTimer * 3.14159f);

        glm::vec4 cloudColor = glm::mix(glm::vec4(1.0f), pastelGreen, colorMix);
        glm::vec4 coinColor = glm::mix(glm::vec4(1.0f), vibrantGreen, colorMix);

        textColor = glm::mix(textColor, vibrantGreen, colorMix);

        if (m_CoinCloudIcon) {
            Renderer2D::DrawQuad(cloudPos, cloudSize, m_CoinCloudIcon, cloudColor, { 0.0f, 1.0f }, { 1.0f, 0.0f });
        }

        glm::vec2 animatedCoinPos = { coinPos.x, coinPos.y - bounceOffset };
        Renderer2D::DrawQuad(animatedCoinPos, coinSize, iconTex, coinColor, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    }
    else {
        BubblyUI::DrawBubblyImage(m_BubblyStates, "CloudIcon", m_CoinCloudIcon, cloudPos, cloudSize, dt, false, 1.05f, false);
        BubblyUI::DrawBubblyImage(m_BubblyStates, "CoinIcon", iconTex, coinPos, coinSize, dt, false, 1.05f, false);
    }

    float currentBounce = (m_CoinAnimTimer > 0.0f && !isWarning) ? std::sin(m_CoinAnimTimer * 3.14159f) * (40.0f * baseScale) : 0.0f;

    float textDrawX = textPos.x + shakeOffsetX;
    float textDrawY = coinPos.y - currentBounce + (coinSize.y * 0.5f) - baselineOffset + (textHeight * 0.25f);

    Gui::DrawGuiText(text, { std::floor(textDrawX + 3.0f), std::floor(textDrawY + 3.0f) }, textScale, { 0.0f, 0.0f, 0.0f, 0.6f });
    Gui::DrawGuiText(text, { std::floor(textDrawX),        std::floor(textDrawY) }, textScale, textColor);
}

void GameGuiLayer::DrawIngredientClouds(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt)
{
    if (!m_CornerIcon) return;
    if (!m_RecipeBookPanel.IsOpen()) {
        m_IngredientsCarousel.OnUpdate(dt);
        m_MachinesCarousel.OnUpdate(dt);
    }
}

void GameGuiLayer::DrawOrderTickets(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt) {
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
            // Usuwamy karteczk� tylko, gdy klient odchodzi, albo jako� uciek� z kolejki
            if (!custScript || custScript->IsPendingDestroy ||
                (custScript->IsServed && custScript->State != CustomerState::LeavingReaction)) {
                it = m_ActiveOrderTickets.erase(it);
                continue;
            }
        }
        ++it;
    }

    // --- STRUKTURA DO ZARZ�DZANIA PI�KNYMI ANIMACJAMI KARTECZEK ---
    struct TicketAnimState {
        float introTimer = 0.0f;
        float currentY = -1.0f;
        float currentHeight = -1.0f;
    };
    static std::unordered_map<std::size_t, TicketAnimState> s_TicketStates;

    // Czyszczenie usuni�tych karteczek z pami�ci
    for (auto it = s_TicketStates.begin(); it != s_TicketStates.end(); ) {
        bool found = false;
        for (auto& cust : m_ActiveOrderTickets) {
            if (cust.id == it->first) { found = true; break; }
        }
        if (!found) it = s_TicketStates.erase(it);
        else ++it;
    }

    float targetY = gameY + (15.0f * baseScale);
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

        bool isFirst = (i == 0);
        float targetHeight = isFirst ? (220.0f * baseScale) : (140.0f * baseScale);
        bool isHelper = (tagComp->Tag.find("HelperCustomer") != std::string::npos);

        std::shared_ptr<Texture> ticketTex = isHelper ? m_HelperOrderTex : m_CustomerOrderTex;
        if (!ticketTex) ticketTex = m_BookCloudIcon;

        if (ticketTex) {
            // --- LOGIKA ANIMACJI P�YNNEGO PRZESUWANIA I WJE�D�ANIA ---
            auto& state = s_TicketStates[custEntity.id];

            // Wjazd z boku
            state.introTimer += dt * 3.5f;
            if (state.introTimer > 1.0f) state.introTimer = 1.0f;

            // Animacja przesuwania w pionie (gdy jedna karteczka znika, reszta p�ynnie jedzie do g�ry)
            if (state.currentY < 0.0f) state.currentY = targetY; // Pojawia si� od razu na swoim miejscu
            else state.currentY += (targetY - state.currentY) * dt * 12.0f; // Mi�kki ruch na now� pozycj�

            // Animacja ro�ni�cia (gdy karteczka staje si� t� "pierwsz�" i najwa�niejsz�)
            if (state.currentHeight < 0.0f) state.currentHeight = targetHeight;
            else state.currentHeight += (targetHeight - state.currentHeight) * dt * 10.0f;

            // Wyliczanie pi�knego wyhamowania
            float easeIn = 1.0f - std::pow(1.0f - state.introTimer, 3.0f);
            float offsetX = (1.0f - easeIn) * (300.0f * baseScale); // Startuje 300px z prawej kraw�dzi

            glm::vec2 ticketSize = GuiUtils::CalculateAspectSize(ticketTex, state.currentHeight);
            glm::vec2 ticketPos = { gameX + gameWidth - ticketSize.x - rightMargin + offsetX, state.currentY };

            glm::vec4 ticketColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            if (custScript && custScript->State == CustomerState::LeavingReaction) {
                if (custScript->m_WasCorrect)
                    ticketColor = { 0.75f, 1.0f, 0.75f, 1.0f }; // Jasna pastelowa ziele�
                else
                    ticketColor = { 1.0f, 0.75f, 0.75f, 1.0f }; // Malinowy r�
            }

            // Renderowanie samej karteczki
            Renderer2D::DrawQuad(ticketPos, ticketSize, ticketTex, ticketColor, { 0.0f, 1.0f }, { 1.0f, 0.0f });
            // --- NOWE: Identyczny, pulsujący blask jak w książce z przepisami ---
            if (isFirst && m_OrderHintTimer > 0.0f && !isHelper) {
                // Obliczamy płynne zanikanie (tak samo jak dla tekstu)
                float alphaFade = 1.0f;
                if (m_OrderHintTimer > 9.5f) { alphaFade = (10.0f - m_OrderHintTimer) / 0.5f; }
                else if (m_OrderHintTimer < 0.5f) { alphaFade = m_OrderHintTimer / 0.5f; }
                alphaFade = std::clamp(alphaFade, 0.0f, 1.0f);

                // Matematyka fali przeniesiona kropka w kropkę z RecipeBookPanel
                float timeNow = glfwGetTime();
                float wave = (std::sin(timeNow * 6.0f) + 1.0f) * 0.5f;
                float flashSpike = std::pow(wave, 4.0f);

                float glowScale = 1.0f + (flashSpike * 0.08f);
                glm::vec2 glowSize = ticketSize * glowScale;
                glm::vec2 glowPos = {
                    ticketPos.x - (glowSize.x - ticketSize.x) * 0.5f,
                    ticketPos.y - (glowSize.y - ticketSize.y) * 0.5f
                };

                // Kolor z uwzględnieniem fali i ogólnego zanikania podpowiedzi
                glm::vec4 flashColor = { 1.0f, 0.65f, 0.95f, flashSpike * 0.95f * alphaFade };
                Renderer2D::DrawQuad(glowPos, glowSize, ticketTex, flashColor, { 0.0f, 1.0f }, { 1.0f, 0.0f });
            }
            // ------


            if (custScript) {
                // Magiczna interpolacja: zawarto�� w �rodku ro�nie dok�adnie w tym samym tempie co papierowa karteczka!
                float t = (state.currentHeight - 140.0f * baseScale) / (80.0f * baseScale);
                t = std::clamp(t, 0.0f, 1.0f);

                // --- LAMBDA POMOCNICZA DO IKON SK�ADNIK�W ---
                auto getIngredientIcon = [&](IngredientType type) -> std::shared_ptr<Texture> {
                    switch (type) {
                    case IngredientType::Tomato:      return m_TomatoIcon;
                    case IngredientType::Cheese:      return m_CheeseIcon;
                    case IngredientType::Ham:         return m_HamIcon;
                    case IngredientType::Milk:        return m_MilkIcon;
                    case IngredientType::Flour:       return m_FlourIcon;
                    case IngredientType::Mozzarella:  return m_MozzarellaIcon;
                    case IngredientType::Egg:         return m_EggIcon;
                    case IngredientType::Apple:       return m_AppleIcon;
                    case IngredientType::Strawberry:  return m_StrawberryIcon;
                    case IngredientType::CoffeeBeans: return m_CoffeeBeanIcon;
                    case IngredientType::Raspberry:   return m_RaspberryIcon;
                    case IngredientType::SleepyDust:  return m_StarPowderIcon;
                    case IngredientType::Potato:      return m_PotatoIcon;
                    default: return m_QuestionMarkIcon;
                    }
                    };

                // --- POBIERANIE IKON NA PODSTAWIE WYMAGA� ---
                std::shared_ptr<Texture> primaryIcon = getIngredientIcon(custScript->WantedIngredient);
                std::shared_ptr<Texture> secondaryIcon = nullptr;

                if (custScript->SecondaryReq.RequirementType == OrderSecondaryRequirement::Type::Ingredient) {
                    secondaryIcon = getIngredientIcon(custScript->SecondaryReq.RequiredIngredient);
                }
                else if (custScript->SecondaryReq.RequirementType == OrderSecondaryRequirement::Type::Machine) {
                    secondaryIcon = AssetManager::GetTexture(custScript->SecondaryReq.MachineIconPath);
                    if (!secondaryIcon) secondaryIcon = m_QuestionMarkIcon; // Zabezpieczenie przed brakiem pliku
                }

                if (primaryIcon) {
                    float pIconH = glm::mix(35.0f * baseScale, 65.0f * baseScale, t);
                    glm::vec2 pSize = GuiUtils::CalculateAspectSize(primaryIcon, pIconH);

                    glm::vec2 pPos = {
                        ticketPos.x + (ticketSize.x - pSize.x) * 0.5f,
                        ticketPos.y + (ticketSize.y - pSize.y) * 0.35f
                    };

                    Renderer2D::DrawQuad(pPos, pSize, primaryIcon, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });

                    if (secondaryIcon) {
                        float sIconH = pIconH * 0.75f;
                        glm::vec2 sSize = GuiUtils::CalculateAspectSize(secondaryIcon, sIconH);

                        float offsetXMultiplier = 0.25f; 

                        if (primaryIcon == m_HamIcon) {
                            offsetXMultiplier = 0.65f; 
                        }

                        glm::vec2 sPos = {
                            pPos.x + pSize.x - (sSize.x * offsetXMultiplier),
                            pPos.y + pSize.y - (sSize.y * 0.65f)
                        };

                        Renderer2D::DrawQuad(sPos, sSize, secondaryIcon, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
                    }

                    if (!isHelper) {
                        std::string rewardText = std::to_string((int)custScript->OrderPrice);
                        float textScale = glm::mix(0.55f * baseScale, 0.75f * baseScale, t);
                        float textWidth = Gui::MeasureTextWidth(rewardText, textScale);

                        float textYOffset = glm::mix(12.0f * baseScale, 18.0f * baseScale, t);
                        glm::vec2 textPos = {
                            ticketPos.x + (ticketSize.x - textWidth) * 0.5f,
                            ticketPos.y + ticketSize.y - textYOffset * 2.75f
                        };

                        Gui::DrawGuiText(rewardText, { textPos.x + 2.0f, textPos.y + 2.0f }, textScale, { 0.1f, 0.2f, 0.1f, 0.7f });
                        Gui::DrawGuiText(rewardText, textPos, textScale, { 1.0f, 1.0f, 1.0f, 1.0f });
                    }
                }
            }

            // --- NOWE: Rysowanie pływającego tekstu z podpowiedzią ---
            if (isFirst && m_OrderHintTimer > 0.0f && !isHelper) {
                float textAlpha = 1.0f;
                // Płynne pojawianie (przez pierwsze 0.5s) i znikanie (przez ostatnie 0.5s)
                if (m_OrderHintTimer > 9.5f) { textAlpha = (10.0f - m_OrderHintTimer) / 0.5f; }
                else if (m_OrderHintTimer < 0.5f) { textAlpha = m_OrderHintTimer / 0.5f; }
                textAlpha = std::clamp(textAlpha, 0.0f, 1.0f);

                std::string line1 = "Remember to serve fully prepared dishes";
                std::string line2 = "including both requested items!";
                float hintScale = 1.0f * baseScale;

                float timeNow = glfwGetTime();
                float floatOffset = std::sin(timeNow * 2.2f) * 5.0f * baseScale; // efekt pływania

                float w1 = Gui::MeasureTextWidth(line1, hintScale);
                float w2 = Gui::MeasureTextWidth(line2, hintScale);
                float maxW = std::max(w1, w2);

                float textH = Gui::MeasureTextHeight("A", hintScale);
                float lineSpacing = textH * 1.3f;
                float totalH = textH + lineSpacing;

                // Ustawiamy tekst po lewej stronie od karteczki
                glm::vec2 blockPos = {
                    ticketPos.x - maxW - 25.0f * baseScale,
                    ticketPos.y + (state.currentHeight - totalH) * 0.5f + floatOffset
                };

                float line1X = blockPos.x + (maxW - w1) * 0.5f;
                float line2X = blockPos.x + (maxW - w2) * 0.5f;

                glm::vec4 shadowColor = { 0.0f, 0.0f, 0.0f, 0.4f * textAlpha };
                glm::vec4 textColor = { 1.0f, 1.0f, 1.0f, 1.0f * textAlpha };

                Gui::DrawGuiText(line1, { line1X + 1.5f, blockPos.y + 1.5f }, hintScale, shadowColor);
                Gui::DrawGuiText(line1, { line1X, blockPos.y }, hintScale, textColor);
                Gui::DrawGuiText(line2, { line2X + 1.5f, blockPos.y + lineSpacing + 1.5f }, hintScale, shadowColor);
                Gui::DrawGuiText(line2, { line2X, blockPos.y + lineSpacing }, hintScale, textColor);
            }

            if (custScript && custScript->State == CustomerState::LeavingReaction && custScript->m_WasCorrect && custScript->AwardedTip > 0.0f) {
                std::string tipText = std::to_string((int)custScript->AwardedTip) + "$ Tip!";
                float tipScale = 0.8f * baseScale; 
                float tipW = Gui::MeasureTextWidth(tipText, tipScale);

                float timeNow = glfwGetTime();
                float floatY = std::sin(timeNow * 6.0f) * 6.0f * baseScale;

                glm::vec2 tipPos = {
                    ticketPos.x + (ticketSize.x - tipW) * 0.8f + 30.0f ,
                    ticketPos.y + floatY + 170.0f
                };

                glm::vec4 tipColor = { 1.0f, 0.85f, 0.2f, 1.0f };
                glm::vec4 shadowColor = { 0.0f, 0.0f, 0.0f, 0.7f };

                Gui::DrawGuiText(tipText, { tipPos.x + 2.5f, tipPos.y + 2.5f }, tipScale, shadowColor);
                Gui::DrawGuiText(tipText, tipPos, tipScale, tipColor);
            }

            targetY += state.currentHeight + (10.0f * baseScale);
        }
    }
}

void GameGuiLayer::OnUpdate(Timestep ts)
{
#ifdef CS_DISTRIBUTION
    if (!m_IsVisible) return;
#endif

    Input::SetUICaptureMouse(false);
    if (m_RecipeBookPanel.IsOpen() || m_IsGamePaused) Input::SetUICaptureMouse(true);

    if (m_IsLevelIntro && !m_FirstOrderTaken) {
        Input::SetUICaptureMouse(true);
    }

    if (m_IsEventIntro && m_EventIntroTimer > 0.0f) {
        Input::SetUICaptureMouse(true);
    }

    Gui::BeginFrame();
    Gui::UpdateDeltaTime(ts.GetSeconds());
    float dt = ts.GetSeconds();

    if (m_CoinAnimTimer > 0.0f) {
        m_CoinAnimTimer -= dt;
        if (m_CoinAnimTimer < 0.0f) m_CoinAnimTimer = 0.0f;
    }

    if (m_OrderHintTimer > 0.0f) {
        m_OrderHintTimer -= dt;
        if (m_OrderHintTimer < 0.0f) m_OrderHintTimer = 0.0f;
    }

    m_ActiveScene = SceneManager::GetActiveScene();

    // ==============================================================
    // KINEMATYCZNE WPROWADZENIE (Śledzenie Kelnera)
    // ==============================================================
    if (m_IsLevelIntro && m_ActiveScene && !GameManagerScript::s_IsTutorialMode) {
        auto* camera = m_ActiveScene->GetCamera();
        if (camera) {
            // Szukamy encji kelnera na scenie
            Entity waiterEntity = { std::numeric_limits<std::size_t>::max(), 0 };
            auto* nscArray = m_ActiveScene->GetWorld().GetComponentVector<NativeScriptComponent>();
            if (nscArray) {
                for (size_t i = 0; i < nscArray->dense.size(); ++i) {
                    for (auto& s : nscArray->dense[i].Scripts) {
                        if (s.Name == "WaiterScript") {
                            waiterEntity = nscArray->reverse[i];
                            break;
                        }
                    }
                    if (waiterEntity.id != std::numeric_limits<std::size_t>::max()) break;
                }
            }

            // --- NOWE: Szukamy, czy na scenie jest już jakikolwiek klient ---
            bool customerExists = false;
            if (nscArray) {
                for (size_t i = 0; i < nscArray->dense.size(); ++i) {
                    for (auto& s : nscArray->dense[i].Scripts) {
                        if (s.Name == "CustomerScript") {
                            customerExists = true;
                            break;
                        }
                    }
                    if (customerExists) break;
                }
            }

            glm::vec3 targetPos = glm::vec3(0.0f);
            float targetZoom = 45.0f;

            // Zbliżenie na kelnera TYLKO, gdy klient wszedł już do restauracji
            if (!m_FirstOrderTaken && customerExists && waiterEntity.id != std::numeric_limits<std::size_t>::max()) {
                auto* tf = m_ActiveScene->GetWorld().GetComponent<TransformComponent>(waiterEntity);
                if (tf) {
                    targetPos = tf->GetPosition();
                    targetZoom = 25.0f; // Miękkie zbliżenie
                }
            }

            // Miękka, płynna interpolacja kamery ("Smooth Follow")
            camera->TargetPosition += (targetPos - camera->TargetPosition) * (dt * 2.5f);
            camera->Zoom += (targetZoom - camera->Zoom) * (dt * 2.5f);

            // Wyłącz cutscenkę, gdy kelner przyjął zamówienie a kamera wróciła do normy
            if (m_FirstOrderTaken && std::abs(camera->Zoom - 45.0f) < 0.5f && glm::length(camera->TargetPosition) < 0.5f) {
                camera->Zoom = 45.0f;
                camera->TargetPosition = glm::vec3(0.0f);
                m_IsLevelIntro = false;

                // --- NOWE: Uruchamiamy timer podpowiedzi ---
                if (!m_HasShownOrderHint) {
                    m_OrderHintTimer = 10.0f; // Będzie widoczne przez 10 sekund
                    m_HasShownOrderHint = true;
                }
            }
        }
    }

    // ==============================================================
    // KINEMATYCZNE WPROWADZENIE EVENTU (Wyspa)
    // ==============================================================
    if (GameManagerScript::s_Instance && GameManagerScript::s_Instance->GetQuestState() == QuestEventState::WaitingForAccept) {
        if (!m_HasShownEventIntro && !GameManagerScript::s_IsTutorialMode) {
            m_IsEventIntro = true;
            m_HasShownEventIntro = true;
            m_EventIntroTimer = 3.0f; // Kamera będzie podziwiać wyspę przez 3 sekundy
        }
    }

    if (m_IsEventIntro && m_ActiveScene) {
        auto* camera = m_ActiveScene->GetCamera();
        if (camera) {
            // Szukamy wysepki na mapie (ma tag "event_78")
            Entity islandEntity = { std::numeric_limits<std::size_t>::max(), 0 };
            auto* tags = m_ActiveScene->GetWorld().GetComponentVector<TagComponent>();
            if (tags) {
                for (size_t i = 0; i < tags->dense.size(); ++i) {
                    if (tags->dense[i].Tag == "event_78") {
                        islandEntity = tags->reverse[i];
                        break;
                    }
                }
            }

            glm::vec3 targetPos = glm::vec3(0.0f);
            float targetZoom = 35.0f; // Delikatne przybliżenie (domyślne to 45.0f)

            if (islandEntity.id != std::numeric_limits<std::size_t>::max()) {
                auto* tf = m_ActiveScene->GetWorld().GetComponent<TransformComponent>(islandEntity);
                if (tf) targetPos = tf->GetPosition();
            }

            if (m_EventIntroTimer > 0.0f) {
                m_EventIntroTimer -= dt;
                // Kamera podąża za (potencjalnie lecącą) wysepką
                camera->TargetPosition += (targetPos - camera->TargetPosition) * (dt * 3.5f);
                camera->Zoom += (targetZoom - camera->Zoom) * (dt * 3.0f);
            }
            else {
                // Koniec pokazu - kamera wraca do centrum mapy i standardowego oddalenia
                camera->TargetPosition += (glm::vec3(0.0f) - camera->TargetPosition) * (dt * 3.5f);
                camera->Zoom += (45.0f - camera->Zoom) * (dt * 3.0f);

                // Gdy kamera wróci na miejsce, całkowicie zdejmujemy blokady
                if (std::abs(camera->Zoom - 45.0f) < 0.5f && glm::length(camera->TargetPosition) < 0.5f) {
                    camera->Zoom = 45.0f;
                    camera->TargetPosition = glm::vec3(0.0f);
                    m_IsEventIntro = false;
                }
            }
        }
    }
    // ==============================================================


#ifdef CS_DISTRIBUTION
    float gameX = 0.0f, gameY = 0.0f, gameWidth = m_ViewportWidth, gameHeight = m_ViewportHeight;
#else
    float gameX = 200.0f, gameY = 30.0f, gameWidth = m_ViewportWidth - 500.0f, gameHeight = m_ViewportHeight - 230.0f;
#endif

    if (gameWidth <= 0.0f || gameHeight <= 0.0f) return;
    float baseScale = std::max(gameHeight / 1080.0f, 0.5f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glScissor((int)gameX, (int)(m_ViewportHeight - (gameY + gameHeight)), (int)gameWidth, (int)gameHeight);

    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);

    if (m_BuildModePanel.IsActive() && m_PausePanel && !m_PausePanel->IsPaused()) {
        m_BuildModePanel.DrawActiveGrid(m_ActiveScene, gameX, gameY, gameWidth, gameHeight, baseScale);
    }

    Renderer2D::BeginScene(uiProj);

    if (m_BuildModePanel.IsActive() && m_PausePanel && !m_PausePanel->IsPaused()) {
        m_BuildModePanel.DrawOverlay(gameX, gameY, gameWidth, gameHeight, baseScale);
    }

    bool isBookOpen = m_RecipeBookPanel.IsOpen();
    bool isPausedBlocked = m_IsGamePaused && !m_BuildModePanel.IsActive();
    bool isPlayMode = !m_IsGamePaused && !m_BuildModePanel.IsActive();

    if (!m_LevelCompletedPanel.IsOpen())
    {
        if (isPlayMode && !isBookOpen && m_ActiveScene) {
            auto mousePos = Input::GetMousePosition();
            float mouseX = mousePos.first;
            float mouseY = mousePos.second;

#ifndef CS_DISTRIBUTION
            mouseX -= gameX;
            mouseY -= gameY;
#endif

            auto* camera = m_ActiveScene->GetCamera();
            if (camera && mouseX >= 0 && mouseY >= 0 && mouseX <= gameWidth && mouseY <= gameHeight) {
                float aspect = gameWidth / (gameHeight > 0.0f ? gameHeight : 1.0f);
                float orthoSize = 10.0f * (camera->Zoom / 45.0f);
                glm::mat4 proj = glm::ortho(-aspect * orthoSize, aspect * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
                glm::mat4 view = camera->GetViewMatrix();

                Ray ray = Physics::CastRayFromMouse(mouseX, mouseY, gameWidth, gameHeight, proj, view);
                Entity hoveredEntity = Physics::GetHoveredEntity(ray, m_ActiveScene, true, true);

                if (hoveredEntity.id != std::numeric_limits<std::size_t>::max()) {
                    auto* nsc = m_ActiveScene->GetWorld().GetComponent<NativeScriptComponent>(hoveredEntity);

                    if (!nsc) {
                        auto* rel = m_ActiveScene->GetWorld().GetComponent<RelationshipComponent>(hoveredEntity);
                        if (rel && rel->Parent != std::numeric_limits<std::size_t>::max()) {
                            Entity parentEntity = { rel->Parent, 0 };
                            nsc = m_ActiveScene->GetWorld().GetComponent<NativeScriptComponent>(parentEntity);
                        }
                    }

                    if (nsc) {
                        for (auto& s : nsc->Scripts) {
                            if (m_RecipeBookPanel.IsOpen() || m_IsGamePaused) return;
                            if (s.Instance) {
                                s.Instance->OnHoverCursor();
                            }
                        }
                    }
                }
            }
        }

        DrawCrateHoverInfo(gameX, gameY, gameWidth, gameHeight, baseScale, dt);
        DrawMachineWarningInfo(gameX, gameY, gameWidth, gameHeight, baseScale, dt);
        DrawHelperHint(gameX, gameY, gameWidth, gameHeight, baseScale);

        if (!GameManagerScript::s_IsTutorialMode)
        {
            DrawMushroomBubble(gameX, gameY, gameWidth, gameHeight, baseScale);
            DrawQuestPanel(gameX, gameY, gameWidth, gameHeight, baseScale, isPlayMode);
            DrawIngredientClouds(gameX, gameY, gameWidth, gameHeight, baseScale, dt);
            m_RecipeBookPanel.Draw(gameX, gameY, gameWidth, gameHeight, baseScale, dt, m_IsGamePaused);
            DrawCustomerOrders(gameX, gameY, gameWidth, gameHeight, baseScale);
            DrawOrderTickets(gameX, gameY, gameWidth, gameHeight, baseScale, dt);
            DrawPackageHoverInfo(gameX, gameY, gameWidth, gameHeight, baseScale, dt);
            m_BuildModePanel.DrawButton(gameX, gameY, gameWidth, gameHeight, baseScale, dt, isPausedBlocked || isBookOpen);
            m_BuildModePanel.DrawPanel(gameX, gameY, gameWidth, gameHeight, baseScale, dt);
            DrawSpeedUpButton(gameX, gameY, gameWidth, gameHeight, baseScale, dt, isPausedBlocked || isBookOpen);

            if (m_CoinIcon) {
                if (GameManagerScript::s_Instance && GameManagerScript::s_Instance->m_MoneyWarningTimer > 0.0f) {
                    GameManagerScript::s_Instance->m_MoneyWarningTimer -= dt;
                }

                if (m_LastMoney == -1 && GameManagerScript::s_Instance)
                    m_CurrentMoney = m_LastMoney = GameManagerScript::s_Instance->GetMoney();
                m_MoneyStr = std::to_string(m_CurrentMoney);
                float textScale = 2.0f * baseScale;
                glm::vec2 textPos = {
                    gameX + (gameWidth - (80.0f * baseScale + 8.0f * baseScale + Gui::MeasureTextWidth(m_MoneyStr, textScale))) * 0.5f + 88.0f * baseScale,
                    gameY + 55.0f * baseScale
                };
                DrawIconWithText(m_MoneyStr, m_CoinIcon, textPos, textScale, baseScale, dt);
            }
        }

        if (GameManagerScript::s_IsTutorialMode && GameManagerScript::s_ShowTutorialDialog)
        {
            float textScale = 1.6f * baseScale;
            std::string speaker = GameManagerScript::s_TutorialSpeaker;
            std::string fullText = GameManagerScript::s_TutorialText;
            int revealedCount = GameManagerScript::s_TutorialCharsRevealed;

            float maxLineWidth = gameWidth * 0.70f;

            float dialogY = GameManagerScript::s_TutorialDialogIsBottom ?
                (gameY + gameHeight * 0.75f) :
                (gameY + gameHeight * 0.15f);
            float dialogX = gameX + gameWidth * 0.5f;

            if (GameManagerScript::s_TutorialTrackedEntity.id != std::numeric_limits<std::size_t>::max() && m_ActiveScene) {
                auto* tf = m_ActiveScene->GetWorld().GetComponent<TransformComponent>(GameManagerScript::s_TutorialTrackedEntity);
                auto* camera = m_ActiveScene->GetCamera();
                if (tf && camera) {
                    glm::vec3 worldPos = tf->GetPosition() + GameManagerScript::s_TutorialTrackedOffset;
                    glm::mat4 view = camera->GetViewMatrix();
                    float currentAspect = gameWidth / (gameHeight > 0.0f ? gameHeight : 1.0f);
                    float orthoSize = 10.0f * (camera->Zoom / 45.0f);
                    glm::mat4 proj3D = glm::ortho(-currentAspect * orthoSize, currentAspect * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
                    glm::mat4 viewProj = proj3D * view;

                    glm::vec4 clipSpace = viewProj * glm::vec4(worldPos, 1.0f);
                    if (clipSpace.w > 0.0f) {
                        glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;
                        dialogX = gameX + (ndc.x + 1.0f) * 0.5f * gameWidth;
                        dialogY = gameY + (1.0f - ndc.y) * 0.5f * gameHeight;
                    }
                }
            }

            float speakerScale = 2.0f * baseScale;
            float speakerWidth = Gui::MeasureTextWidth(speaker, speakerScale);
            float speakerX = dialogX - speakerWidth * 0.5f;

            Gui::DrawGuiText(speaker, { speakerX + 2.0f, dialogY - 50.0f * baseScale + 2.0f }, speakerScale, { 0.0f, 0.0f, 0.0f, 0.4f });
            Gui::DrawGuiText(speaker, { speakerX, dialogY - 50.0f * baseScale }, speakerScale, GameManagerScript::s_TutorialSpeakerColor);

            std::vector<std::string> lines;
            std::string currentLine = "";
            std::string currentWord = "";

            for (size_t i = 0; i <= fullText.size(); ++i) {
                char c = (i < fullText.size()) ? fullText[i] : ' ';
                if (c == ' ' || i == fullText.size()) {
                    std::string testLine = currentLine.empty() ? currentWord : (currentLine + " " + currentWord);
                    if (Gui::MeasureTextWidth(testLine, textScale) > maxLineWidth) {
                        lines.push_back(currentLine);
                        currentLine = currentWord;
                    }
                    else {
                        currentLine = testLine;
                    }
                    currentWord = "";
                }
                else {
                    currentWord += c;
                }
            }
            if (!currentLine.empty()) lines.push_back(currentLine);

            float currentY = dialogY;
            float lineHeight = Gui::MeasureTextHeight("A", textScale) * 1.5f;
            int charsLeftToDraw = revealedCount;

            float maxActualWidth = 0.0f;
            float lastLineY = currentY;

            for (const auto& line : lines) {
                if (charsLeftToDraw <= 0) break;

                int charsInThisLine = std::min((int)line.length(), charsLeftToDraw);
                std::string drawnLine = line.substr(0, charsInThisLine);
                charsLeftToDraw -= charsInThisLine;

                float totalLineWidth = Gui::MeasureTextWidth(line, textScale);
                float lineStartX = dialogX - totalLineWidth * 0.5f;

                Gui::DrawGuiText(drawnLine, { lineStartX + 2.0f, currentY + 2.0f }, textScale, { 0.0f, 0.0f, 0.0f, 0.5f });
                Gui::DrawGuiText(drawnLine, { lineStartX, currentY }, textScale, glm::vec4(1.0f));

                if (totalLineWidth > maxActualWidth) {
                    maxActualWidth = totalLineWidth;
                }

                lastLineY = currentY;
                currentY += lineHeight;

                if (charsInThisLine < line.length() || charsLeftToDraw <= 0) break;
                charsLeftToDraw--;
            }

            if (GameManagerScript::s_TutorialIconAlpha > 0.0f) {
                static std::shared_ptr<Texture> s_LMBIcon = AssetManager::GetTexture("assets://UI/leftMouse.png");
                if (s_LMBIcon) {
                    float iconSizeY = 80.0f * baseScale;
                    glm::vec2 iconSize = GuiUtils::CalculateAspectSize(s_LMBIcon, iconSizeY);

                    float blockRightEdge = dialogX + maxActualWidth * 0.5f;
                    float blockCenterY = (dialogY + lastLineY) * 0.5f;

                    glm::vec2 iconPos = {
                        blockRightEdge + 25.0f * baseScale,
                        blockCenterY + (lineHeight * 0.3f) - (iconSize.y * 0.5f)
                    };

                    float timeNow = glfwGetTime();
                    iconPos.y += std::sin(timeNow * 5.0f) * 5.0f * baseScale;

                    Renderer2D::DrawQuad(iconPos, iconSize, s_LMBIcon, { 1.0f, 1.0f, 1.0f, GameManagerScript::s_TutorialIconAlpha }, { 0.0f, 1.0f }, { 1.0f, 0.0f });

                    Renderer2D::EndScene();

                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

                    Renderer2D::BeginScene(uiProj);

                    float wave = (std::sin(timeNow * 4.0f) + 1.0f) * 0.5f;
                    float flashSpike = std::pow(wave, 16.0f);

                    float glowScale = 1.0f + (flashSpike * 0.1f);
                    glm::vec2 glowSize = iconSize * glowScale;
                    glm::vec2 glowPos = {
                        iconPos.x - (glowSize.x - iconSize.x) * 0.5f,
                        iconPos.y - (glowSize.y - iconSize.y) * 0.5f
                    };

                    glm::vec4 flashColor = { 1.0f, 1.0f, 1.0f, flashSpike * GameManagerScript::s_TutorialIconAlpha };

                    Renderer2D::DrawQuad(glowPos, glowSize, s_LMBIcon, flashColor, { 0.0f, 1.0f }, { 1.0f, 0.0f });

                    Renderer2D::EndScene();
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    Renderer2D::BeginScene(uiProj);
                }
            }
        }


        if (GameManagerScript::s_IsTutorialMode && TutorialManagerScript::s_AllowSkip) {
            static float skipHoldProgress = 0.0f;
            static float skipBtnScale = 1.0f;

            static std::shared_ptr<Texture> s_SkipBtnTex = nullptr;
            if (!s_SkipBtnTex) {
                s_SkipBtnTex = AssetManager::GetTexture("assets://UI/skipTutoButton.png");
            }

            float baseH = 65.0f * baseScale;
            float baseW = 200.0f * baseScale;
            if (s_SkipBtnTex && s_SkipBtnTex->GetHeight() > 0) {
                float aspect = (float)s_SkipBtnTex->GetWidth() / (float)s_SkipBtnTex->GetHeight();
                baseW = baseH * aspect;
            }

            glm::vec2 basePos = { gameX + gameWidth - baseW - 30.0f * baseScale, gameY + gameHeight - baseH - 30.0f * baseScale };

            glm::vec2 mouse = Gui::GetMappedMousePos();
            bool isHovered = (mouse.x >= basePos.x && mouse.x <= basePos.x + baseW && mouse.y >= basePos.y && mouse.y <= basePos.y + baseH);

            skipBtnScale += ((isHovered ? 1.08f : 1.0f) - skipBtnScale) * (dt * 15.0f);

            float btnW = baseW * skipBtnScale;
            float btnH = baseH * skipBtnScale;
            glm::vec2 btnPos = basePos - glm::vec2((btnW - baseW) * 0.5f, (btnH - baseH) * 0.5f);
            glm::vec2 btnSize = { btnW, btnH };

            if (isHovered && Input::IsMouseButtonPressed(0)) {
                skipHoldProgress += dt * 0.66f;
                if (skipHoldProgress >= 1.0f) {
                    skipHoldProgress = 0.0f;
                    g_TriggerCloudTransition = true;
                }
            }
            else {
                skipHoldProgress -= dt * 2.0f;
                if (skipHoldProgress < 0.0f) skipHoldProgress = 0.0f;
            }

            if (s_SkipBtnTex) {
                Renderer2D::DrawQuad(btnPos, btnSize, s_SkipBtnTex, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), { 0.0f, 1.0f }, { 1.0f, 0.0f });

                if (skipHoldProgress > 0.0f) {
                    glm::vec2 fillSize = { btnSize.x * skipHoldProgress, btnSize.y };
                    glm::vec2 fillUv1 = { skipHoldProgress, 0.0f };
                    Renderer2D::DrawQuad(btnPos, fillSize, s_SkipBtnTex, glm::vec4(0.85f, 0.55f, 1.0f, 1.0f), { 0.0f, 1.0f }, fillUv1);
                }
            }

            if (isHovered) Input::SetUICaptureMouse(true);
        }
    }


    if (m_ShowFPS) {
        static float s_FpsUpdateTimer = 0.0f;
        static int s_LastFPS = 0;

        s_FpsUpdateTimer += dt;
        if (s_FpsUpdateTimer > 0.5f) {
            s_LastFPS = (int)(1.0f / dt);
            s_FpsUpdateTimer = 0.0f;
        }

        std::string fpsText = "FPS: " + std::to_string(s_LastFPS);
        float textScale = 0.65f * baseScale;
        glm::vec2 textPos = { gameX + 15.0f * baseScale, gameY + 25.0f * baseScale };

        Gui::DrawGuiText(fpsText, { textPos.x + 2.0f, textPos.y + 2.0f }, textScale, { 0.0f, 0.0f, 0.0f, 0.8f });
        Gui::DrawGuiText(fpsText, textPos, textScale, { 0.2f, 0.9f, 0.2f, 1.0f });
    }

    Renderer2D::EndScene();
    glDisable(GL_SCISSOR_TEST);

    if (m_BuildModePanel.IsActive()) {
        m_BuildModePanel.UpdatePlacement(m_ActiveScene, gameX, gameY, gameWidth, gameHeight, baseScale);
    }

    if (m_PausePanel && m_PausePanel->IsPaused()) {
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        Renderer2D::BeginScene(uiProj);
        m_PausePanel->OnUpdate(dt);
        m_PausePanel->Draw(baseScale);
        Renderer2D::EndScene();
    }

    if (m_LevelCompletedPanel.IsOpen()) {
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        Renderer2D::BeginScene(uiProj);
        m_LevelCompletedPanel.OnUpdate(dt);
        m_LevelCompletedPanel.Draw(m_ViewportWidth, m_ViewportHeight, baseScale);
        Renderer2D::EndScene();
    }
    static int s_CloudTransState = 0;
    static float s_CloudTransProgress = 0.0f;

    if (g_TriggerCloudTransition && s_CloudTransState == 0) {
        g_TriggerCloudTransition = false;
        s_CloudTransState = 1;
        s_CloudTransProgress = 0.0f;
    }

    if (s_CloudTransState > 0) {
        Input::SetUICaptureMouse(true);
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        Renderer2D::BeginScene(uiProj);

        float speed = 1.5f * dt;
        if (s_CloudTransState == 1) {
            s_CloudTransProgress += speed;
            if (s_CloudTransProgress >= 1.0f) {
                s_CloudTransProgress = 1.0f;
                s_CloudTransState = 2;

                if (m_ActiveScene) {
                    auto* oldCam = m_ActiveScene->GetCamera();
                    if (oldCam) {
                        oldCam->Zoom = 45.0f;
                        oldCam->TargetPosition = glm::vec3(0.0f);
                    }
                }

                GameManagerScript::s_IsTutorialMode = false;
                auto activeScene = SceneManager::NewScene();
                SceneSerializer serializer(activeScene.get());

                if (serializer.Deserialize("assets://levels/level02.json")) {
                    auto windowSize = Input::GetWindowSize();
                    activeScene->SetViewportSize((float)windowSize.first, (float)windowSize.second);
                    Gui::SetScreenSize((float)windowSize.first, (float)windowSize.second);

                    activeScene->SetState(SceneState::Play);
                    activeScene->OnRuntimeStart();

                    auto* cam = activeScene->GetCamera();
                    if (cam) {
                        cam->Zoom = 45.0f;
                        cam->TargetPosition = glm::vec3(0.0f);
                    }

                    Application::Get().GetEventBus().Publish(GameStartedEvent{});
                    Application::Get().GetEventBus().Publish(GameResumedEvent{});
                }
            }
        }
        else if (s_CloudTransState == 2) {
            s_CloudTransProgress -= speed;
            if (s_CloudTransProgress <= 0.0f) {
                s_CloudTransProgress = 0.0f;
                s_CloudTransState = 0;
            }
        }

        if (m_BookCloudIcon) {
            float t = s_CloudTransProgress;
            float easeScale = t * t * (3.0f - 2.0f * t);

            float maxCloudSize = std::max(m_ViewportWidth, m_ViewportHeight) * 3.5f;
            float currentSize = maxCloudSize * easeScale;

            glm::vec2 size = { currentSize, currentSize };
            glm::vec2 pos = { (m_ViewportWidth - currentSize) * 0.5f, (m_ViewportHeight - currentSize) * 0.5f };

            Renderer2D::DrawQuad(pos, size, m_BookCloudIcon, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
        }
        Renderer2D::EndScene();
    }
    glEnable(GL_DEPTH_TEST);
}

void GameGuiLayer::OnEvent(Event& e)
{
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

    if (m_RecipeBookPanel.IsOpen() || (m_BuildModePanel.IsActive() && e.GetEventType() == EventType::MouseScrolled)) {
        if (e.GetEventType() == EventType::MouseButtonPressed ||
            e.GetEventType() == EventType::MouseButtonReleased ||
            e.GetEventType() == EventType::MouseMoved ||
            e.GetEventType() == EventType::MouseScrolled)
        {
            e.Handled = true;
        }
    }

    if (m_PausePanel) { m_PausePanel->OnEvent(e); if (e.Handled) return; }
    if (m_LevelCompletedPanel.IsOpen()) {
        m_LevelCompletedPanel.OnEvent(e);
        if (e.Handled) return;
    }
    dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& ev) {
        m_IngredientsCarousel.OnMouseScrolled(ev, m_ViewportWidth, 8);
        m_MachinesCarousel.OnMouseScrolled(ev, m_ViewportWidth, 8);
        return false;
        });

    dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
        if (ev.GetKeyCode() == 292 && ev.GetRepeatCode() == 0) m_ShowFPS = !m_ShowFPS;
        if (ev.GetKeyCode() == 258 && ev.GetRepeatCode() == 0) {
            if (GameManagerScript::s_IsTutorialMode) return true;
            m_BuildModePanel.Toggle();
            return true;
        }
        return false;
        });
}

void GameGuiLayer::DrawQuestPanel(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, bool isPlayMode)
{
    if (m_RecipeBookPanel.IsOpen() || m_IsGamePaused) return;
    if (!GameManagerScript::s_Instance) return;

    QuestEventState state = GameManagerScript::s_Instance->GetQuestState();
    if (state != QuestEventState::WaitingForAccept && state != QuestEventState::QuestActive) return;

    Entity targetEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    float yOffset3D = 2.5f;

    if (state == QuestEventState::WaitingForAccept) {
        auto* tags = m_ActiveScene->GetWorld().GetComponentVector<TagComponent>();
        if (tags) {
            for (size_t i = 0; i < tags->dense.size(); ++i) {
                if (tags->dense[i].Tag == "event_78") {
                    targetEntity = tags->reverse[i];
                    break;
                }
            }
        }
        yOffset3D = 2.5f;
    }
    else if (state == QuestEventState::QuestActive) {
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
        if (targetEntity.id == std::numeric_limits<std::size_t>::max()) {
            auto* tags = m_ActiveScene->GetWorld().GetComponentVector<TagComponent>();
            if (tags) {
                for (size_t i = 0; i < tags->dense.size(); ++i) {
                    if (tags->dense[i].Tag == "naroznikPas") {
                        targetEntity = tags->reverse[i];
                        break;
                    }
                }
            }
        }
        yOffset3D = 0.8f;
    }

    if (targetEntity.id == std::numeric_limits<std::size_t>::max()) return;

    QuestData* activeQuest = GameManagerScript::s_Instance->GetCurrentQuest();
    if (!activeQuest) return;

    auto* transform = m_ActiveScene->GetWorld().GetComponent<TransformComponent>(targetEntity);
    if (!transform) return;

    glm::vec3 boothGlobalPos = transform->GetPosition();
    boothGlobalPos.y += yOffset3D;

    auto* camera = m_ActiveScene->GetCamera();
    if (!camera) return;
    glm::mat4 view = camera->GetViewMatrix();
    float currentAspect = gameWidth / (gameHeight > 0.0f ? gameHeight : 1.0f);
    float orthoSize = 10.0f * (camera->Zoom / 45.0f);
    glm::mat4 proj3D = glm::ortho(-currentAspect * orthoSize, currentAspect * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
    glm::mat4 viewProj3D = proj3D * view;

    glm::vec4 clipSpacePos = viewProj3D * glm::vec4(boothGlobalPos, 1.0f);
    if (clipSpacePos.w <= 0.0f) return;
    glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;

    float boothScreenX = gameX + (ndcSpacePos.x + 1.0f) * 0.5f * gameWidth;
    float boothScreenY = gameY + (1.0f - ndcSpacePos.y) * 0.5f * gameHeight;

    glm::vec2 cloudSize = { 380.0f * baseScale, (state == QuestEventState::WaitingForAccept ? 260.0f : 200.0f) * baseScale };
    glm::vec2 cloudPos = { boothScreenX - cloudSize.x * 0.5f, boothScreenY - cloudSize.y };
    if (cloudPos.x < gameX + 10.0f) cloudPos.x = gameX + 10.0f;
    if (cloudPos.x + cloudSize.x > gameX + gameWidth - 10.0f) cloudPos.x = gameX + gameWidth - cloudSize.x;
    if (cloudPos.y < gameY + 10.0f) cloudPos.y = gameY + 10.0f;

    glm::vec2 mousePos = Gui::GetMappedMousePos();
    float hoverRadius = (state == QuestEventState::WaitingForAccept ? 70.0f : 60.0f) * baseScale;
    float dx = mousePos.x - boothScreenX;
    float dy = mousePos.y - boothScreenY;
    bool isHovering3D = ((dx * dx + dy * dy) <= (hoverRadius * hoverRadius));

    float margin = 30.0f * baseScale;
    bool isHoveringPanel = (mousePos.x >= cloudPos.x - margin && mousePos.x <= cloudPos.x + cloudSize.x + margin &&
        mousePos.y >= cloudPos.y - margin && mousePos.y <= cloudPos.y + cloudSize.y + margin);

    static bool s_wasEventHovered = false;
    bool isHoveringAny = isHovering3D || isHoveringPanel;

    if (isHoveringAny && !s_wasEventHovered) {
        AudioEngine::PlayLoopingSound("assets://sounds/hover_in_game.mp3", 0.15f, false);
        s_wasEventHovered = true;
    }
    else if (!isHoveringAny) {
        s_wasEventHovered = false;
    }

    static bool s_IsQuestPanelVisible = false;

    if (!isHovering3D && !(s_IsQuestPanelVisible && isHoveringPanel)) {
        s_IsQuestPanelVisible = false;
        return;
    }

    s_IsQuestPanelVisible = true;
    Input::SetUICaptureMouse(true);

    if (state == QuestEventState::QuestActive)
    {
        if (!m_EventCloudTex) return;

        float cloudW = 300.0f * baseScale;
        float cloudAspect = (float)m_EventCloudTex->GetHeight() / (float)m_EventCloudTex->GetWidth();
        float cloudH = cloudW * cloudAspect;

        glm::vec2 newCloudPos = { boothScreenX - cloudW * 0.5f, boothScreenY - cloudH * 1.2f };
        if (newCloudPos.x < gameX + 10.0f) newCloudPos.x = gameX + 10.0f;
        if (newCloudPos.x + cloudW > gameX + gameWidth - 10.0f) newCloudPos.x = gameX + gameWidth - cloudW - 10.0f;
        if (newCloudPos.y < gameY + 10.0f) newCloudPos.y = gameY + 10.0f;

        Renderer2D::DrawQuad(newCloudPos, { cloudW, cloudH }, m_EventCloudTex, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });

        float mainCenterX = newCloudPos.x + cloudW * 0.55f;

        float titleScale = 0.78f * baseScale;
        float titleW = Gui::MeasureTextWidth(activeQuest->Title, titleScale);
        Gui::DrawGuiText(activeQuest->Title, { mainCenterX - titleW * 0.61f, newCloudPos.y + cloudH * 0.24f }, titleScale, titleColor);

        float descScale = 0.78f * baseScale;
        float descMaxW = cloudW * 0.75f;

        float myCustomCenterX = mainCenterX - (15.0f * baseScale);

        float curY = newCloudPos.y + cloudH * 0.35f;
        float descLineH = Gui::MeasureTextHeight("A", descScale) * 1.3f;

        {
            std::string desc = activeQuest->Description;
            std::string line;
            std::string word;
            for (size_t ci = 0; ci <= desc.size(); ++ci) {
                char c = (ci < desc.size()) ? desc[ci] : ' ';
                if (c == ' ' || ci == desc.size()) {
                    std::string testLine = line.empty() ? word : (line + " " + word);
                    if (Gui::MeasureTextWidth(testLine, descScale) > descMaxW && !line.empty()) {
                        Gui::DrawGuiText(line, { myCustomCenterX - Gui::MeasureTextWidth(line, descScale) * 0.5f, curY }, descScale, descColor);
                        curY += descLineH;
                        line = word;
                    }
                    else {
                        line = testLine;
                    }
                    word = "";
                }
                else {
                    word += c;
                }
            }
            if (!line.empty())
                Gui::DrawGuiText(line, { myCustomCenterX - Gui::MeasureTextWidth(line, descScale) * 0.5f, curY }, descScale, descColor);
        }

        float slotSize = cloudH * 0.28f;
        glm::vec2 slotPos = { mainCenterX - slotSize * 1.2f, newCloudPos.y + cloudH * 0.58f };

        std::shared_ptr<Texture> dishIcon = m_QuestionMarkIcon;
        if (activeQuest->DishID == "pomidorowa")      dishIcon = AssetManager::GetTexture("assets://UI/TomatoSoup.png");
        else if (activeQuest->DishID == "kanapka")    dishIcon = AssetManager::GetTexture("assets://UI/sandwich.png");
        else if (activeQuest->DishID == "kopytka")    dishIcon = AssetManager::GetTexture("assets://UI/Gnocchi.png");
        else if (activeQuest->DishID == "babeczka")   dishIcon = AssetManager::GetTexture("assets://UI/Cupcake.png");

        if (dishIcon) {
            float iconPadding = slotSize * 0.15f;
            float maxIconSize = slotSize - iconPadding * 2.0f;

            float texAspect = (float)dishIcon->GetWidth() / (float)dishIcon->GetHeight();
            glm::vec2 drawSize = { maxIconSize, maxIconSize };

            if (texAspect > 1.0f) {
                drawSize.y = maxIconSize / texAspect;
            }
            else {
                drawSize.x = maxIconSize * texAspect;
            }

            glm::vec2 iconPos = {
                slotPos.x + (slotSize - drawSize.x) * 0.5f,
                slotPos.y + (slotSize - drawSize.y) * 0.5f
            };

            Renderer2D::DrawQuad(iconPos, drawSize, dishIcon, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
        }

        int delivered = GameManagerScript::s_Instance->GetQuestProgress();
        std::string progressStr = std::to_string(delivered) + "/" + std::to_string(activeQuest->Portions);
        float progressScale = 1.25f * baseScale;
        float progressY = slotPos.y + slotSize * 0.7f - Gui::MeasureTextHeight(progressStr, progressScale) * 1.1f;

        Gui::DrawGuiText(progressStr, { slotPos.x + slotSize + 10.0f * baseScale, progressY }, progressScale, circleColor);

        if (m_EventRewardTex) {
            float rewardAspect = (float)m_EventRewardTex->GetWidth() / (float)m_EventRewardTex->GetHeight();
            float rewardH = cloudH * 0.6f;
            float rewardW = rewardH * rewardAspect;

            glm::vec2 rewardPos = { newCloudPos.x + cloudW - rewardW * 2.3f, newCloudPos.y - rewardH * 0.6f };

            Renderer2D::DrawQuad(rewardPos, { rewardW, rewardH }, m_EventRewardTex, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });

            std::string coinsStr = std::to_string(activeQuest->RewardCoins);
            float coinsScale = 0.8f * baseScale;
            float coinsW = Gui::MeasureTextWidth(coinsStr, coinsScale);

            float bakedCoinCenterX = rewardPos.x + rewardW * 0.33f;
            float bakedCoinCenterY = rewardPos.y + rewardH * 0.52f;

            Gui::DrawGuiText(coinsStr, { bakedCoinCenterX - coinsW * 0.5f + 1.5f, bakedCoinCenterY + 1.5f }, coinsScale, { 0.0f, 0.0f, 0.0f, 0.6f });
            Gui::DrawGuiText(coinsStr, { bakedCoinCenterX - coinsW * 0.5f, bakedCoinCenterY }, coinsScale, { 1.0f, 1.0f, 1.0f, 1.0f });

            std::string flagText = activeQuest->RewardFlag;
            glm::vec4 flagTextColor = { 0.88f, 0.75f, 0.93f, 1.0f };
            float flagTextScale = 0.8f * baseScale;
            float flagTextW = Gui::MeasureTextWidth(flagText, flagTextScale);

            float bakedFlagCenterX = rewardPos.x + rewardW * 0.8f;
            float bakedFlagBottomY = rewardPos.y + rewardH * 0.6f;
            Gui::DrawGuiText(flagText, { bakedFlagCenterX - flagTextW * 0.5f, bakedFlagBottomY }, flagTextScale, flagTextColor);
        }

        return;
    }

    if (!m_QuestCloudTex) return;

    float cloudW = 500.0f * baseScale;
    float cloudH = cloudW * (435.0f / 500.0f);

    glm::vec2 newCloudPos = { boothScreenX - cloudW * 0.5f, boothScreenY - cloudH };
    if (newCloudPos.x < gameX + 10.0f) newCloudPos.x = gameX + 10.0f;
    if (newCloudPos.x + cloudW > gameX + gameWidth - 10.0f) newCloudPos.x = gameX + gameWidth - cloudW - 10.0f;
    if (newCloudPos.y < gameY + 10.0f) newCloudPos.y = gameY + 10.0f;

    Renderer2D::DrawQuad(newCloudPos, { cloudW, cloudH }, m_QuestCloudTex, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });

    float titleScale = 1.3f * baseScale;
    float titleW = Gui::MeasureTextWidth(activeQuest->Title, titleScale);
    glm::vec4 titleColor = { 0.42f, 0.24f, 0.46f, 1.0f };
    Gui::DrawGuiText(activeQuest->Title, { newCloudPos.x + (cloudW - titleW) * 0.5f, newCloudPos.y + cloudH * 0.27f }, titleScale, titleColor);

    float descScale = 1.1f * baseScale;
    glm::vec4 descColor = { 0.56f, 0.37f, 0.66f, 1.0f };
    float descMaxW = cloudW * 0.70f;
    float descY = newCloudPos.x + (cloudW - descMaxW) * 0.5f;
    float curY = newCloudPos.y + cloudH * 0.38f;
    float descLineH = Gui::MeasureTextHeight("A", descScale) * 1.3f;

    {
        std::string desc = activeQuest->Description;
        std::string line;
        std::string word;
        for (size_t ci = 0; ci <= desc.size(); ++ci) {
            char c = (ci < desc.size()) ? desc[ci] : ' ';
            if (c == ' ' || ci == desc.size()) {
                std::string testLine = line.empty() ? word : (line + " " + word);
                if (Gui::MeasureTextWidth(testLine, descScale) > descMaxW && !line.empty()) {
                    Gui::DrawGuiText(line, { newCloudPos.x + (cloudW - Gui::MeasureTextWidth(line, descScale)) * 0.5f, curY }, descScale, descColor);
                    curY += descLineH;
                    line = word;
                }
                else {
                    line = testLine;
                }
                word = "";
            }
            else {
                word += c;
            }
        }
        if (!line.empty())
            Gui::DrawGuiText(line, { newCloudPos.x + (cloudW - Gui::MeasureTextWidth(line, descScale)) * 0.5f, curY }, descScale, descColor);
    }

    float acceptScaleFactor = 0.075f;
    float acceptBaseH = cloudH * acceptScaleFactor;
    float acceptAspect = m_AcceptButtonTex ? (float)m_AcceptButtonTex->GetWidth() / (float)m_AcceptButtonTex->GetHeight() : 1.0f;
    float acceptBaseW = acceptBaseH * acceptAspect;

    float skipScaleFactor = 0.13f;
    float skipBaseH = cloudH * skipScaleFactor;
    float skipAspect = m_SkipButtonTex ? (float)m_SkipButtonTex->GetWidth() / (float)m_SkipButtonTex->GetHeight() : 1.0f;
    float skipBaseW = skipBaseH * skipAspect;

    float buttonsCenterY = newCloudPos.y + cloudH * 0.72f;

    glm::vec2 acceptBasePos = { newCloudPos.x + 30.0f * baseScale, buttonsCenterY - acceptBaseH * 0.5f };
    glm::vec2 skipBasePos = { newCloudPos.x + cloudW - skipBaseW - 35.0f * baseScale, buttonsCenterY - skipBaseH * 0.5f };

    glm::vec2 mouse = Gui::GetMappedMousePos();
    bool acceptHit = (mouse.x >= acceptBasePos.x && mouse.x <= acceptBasePos.x + acceptBaseW && mouse.y >= acceptBasePos.y && mouse.y <= acceptBasePos.y + acceptBaseH);
    bool skipHit = (mouse.x >= skipBasePos.x && mouse.x <= skipBasePos.x + skipBaseW && mouse.y >= skipBasePos.y && mouse.y <= skipBasePos.y + skipBaseH);

    static bool s_wasAcceptHit = false;
    if (acceptHit && !s_wasAcceptHit) {
        AudioEngine::PlayLoopingSound("assets://sounds/hover_in_game.mp3", 0.15f, false);
        s_wasAcceptHit = true;
    }
    else if (!acceptHit) {
        s_wasAcceptHit = false;
    }

    static bool s_wasSkipHit = false;
    if (skipHit && !s_wasSkipHit) {
        AudioEngine::PlayLoopingSound("assets://sounds/hover_in_game.mp3", 0.15f, false);
        s_wasSkipHit = true;
    }
    else if (!skipHit) {
        s_wasSkipHit = false;
    }

    static float s_accScale = 1.0f;
    static float s_skipScale = 1.0f;

    s_accScale += ((acceptHit ? 1.15f : 1.0f) - s_accScale) * 0.2f;
    s_skipScale += ((skipHit ? 1.15f : 1.0f) - s_skipScale) * 0.2f;

    float acceptW = acceptBaseW * s_accScale;
    float acceptH = acceptBaseH * s_accScale;
    glm::vec2 acceptPos = acceptBasePos - glm::vec2((acceptW - acceptBaseW) * 0.5f, (acceptH - acceptBaseH) * 0.5f);

    float skipW = skipBaseW * s_skipScale;
    float skipH = skipBaseH * s_skipScale;
    glm::vec2 skipPos = skipBasePos - glm::vec2((skipW - skipBaseW) * 0.5f, (skipH - skipBaseH) * 0.5f);

    glm::vec4 acceptTint = { 1.0f, 1.0f, 1.0f, 1.0f };
    if (acceptHit) {
        acceptTint = Input::IsMouseButtonPressed(0) ? glm::vec4(0.65f, 0.65f, 0.65f, 1.0f) : glm::vec4(0.85f, 0.85f, 0.85f, 1.0f);
    }

    glm::vec4 skipTint = { 1.0f, 1.0f, 1.0f, 1.0f };
    if (skipHit) {
        skipTint = Input::IsMouseButtonPressed(0) ? glm::vec4(0.65f, 0.65f, 0.65f, 1.0f) : glm::vec4(0.85f, 0.85f, 0.85f, 1.0f);
    }

    if (m_AcceptButtonTex)
        Renderer2D::DrawQuad(acceptPos, { acceptW, acceptH }, m_AcceptButtonTex, acceptTint, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    if (m_SkipButtonTex)
        Renderer2D::DrawQuad(skipPos, { skipW, skipH }, m_SkipButtonTex, skipTint, { 0.0f, 1.0f }, { 1.0f, 0.0f });

    int skipsLeft = GameManagerScript::s_Instance->GetSkipsLeft();
    std::string skipsStr = "(" + std::to_string(skipsLeft) + ")";

    float skipsScale = 0.7f * baseScale * s_skipScale;
    float skipsW = Gui::MeasureTextWidth(skipsStr, skipsScale);

    float skipsX = skipPos.x + skipW * 0.63f;
    float skipsY = skipPos.y + (19.0f * baseScale * s_skipScale);

    glm::vec4 skipsColor = { 157.0f / 255.0f, 113.0f / 255.0f, 180.0f / 255.0f, 1.0f };
    Gui::DrawGuiText(skipsStr, { skipsX, skipsY }, skipsScale, skipsColor);

    std::string coinsStr = std::to_string(activeQuest->RewardCoins);
    float coinsScale = 0.75f * baseScale;
    float coinsW = Gui::MeasureTextWidth(coinsStr, coinsScale);
    float bakedCoinCenterX = newCloudPos.x + cloudW * 0.41f;
    float bakedCoinCenterY = newCloudPos.y + cloudH * 0.78f;

    Gui::DrawGuiText(coinsStr, { bakedCoinCenterX - coinsW * 0.5f + 1.5f, bakedCoinCenterY + 1.5f }, coinsScale, { 0.0f, 0.0f, 0.0f, 0.6f });
    Gui::DrawGuiText(coinsStr, { bakedCoinCenterX - coinsW * 0.5f, bakedCoinCenterY }, coinsScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    std::string flagText = activeQuest->RewardFlag;
    glm::vec4 flagTextColor = { 0.88f, 0.75f, 0.93f, 1.0f };
    float flagTextScale = 0.76f * baseScale;
    float flagTextW = Gui::MeasureTextWidth(flagText, flagTextScale);
    float bakedFlagCenterX = newCloudPos.x + cloudW * 0.63f;
    float bakedFlagBottomY = newCloudPos.y + cloudH * 0.78f;
    Gui::DrawGuiText(flagText, { bakedFlagCenterX - flagTextW * 0.5f, bakedFlagBottomY }, flagTextScale, flagTextColor);

    if (acceptHit && Input::IsMouseButtonJustPressed(0)) {
        AudioEngine::Play("assets://sounds/button_click_in_game.mp3");
        GameManagerScript::s_Instance->AcceptQuest();
    }

    if (skipHit && skipsLeft > 0 && Input::IsMouseButtonJustPressed(0)) {
        AudioEngine::Play("assets://sounds/button_click_in_game.mp3");
        GameManagerScript::s_Instance->SkipQuest();
    }
    std::string collectStr = "Collect all flags!";
    float collectScale = 1.0f * baseScale;
    float collectW = Gui::MeasureTextWidth(collectStr, collectScale);

    float timeNow = glfwGetTime();
    float floatOffset = std::sin(timeNow * 2.2f) * 5.0f * baseScale; // Ten sam efekt pływania co w podpowiedziach

    // Środek pod chmurką
    glm::vec2 collectPos = {
        newCloudPos.x + (cloudW - collectW) * 0.5f,
        newCloudPos.y + cloudH + 15.0f * baseScale + floatOffset
    };

    glm::vec4 shadowColor = { 0.0f, 0.0f, 0.0f, 0.6f };
    glm::vec4 textColor = { 1.0f, 0.85f, 0.2f, 1.0f }; // Dałem przyjemny, złocisty odcień dla klimatu kolekcji!

    Gui::DrawGuiText(collectStr, { collectPos.x + 1.5f, collectPos.y + 1.5f }, collectScale, shadowColor);
    Gui::DrawGuiText(collectStr, collectPos, collectScale, textColor);
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
    float orthoSize = 10.0f * (camera->Zoom / 45.0f);
    glm::mat4 proj3D = glm::ortho(-currentAspect * orthoSize, currentAspect * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
    glm::mat4 viewProj = proj3D * view;

    for (size_t i = 0; i < tags->dense.size(); ++i) {
        std::string tag = tags->dense[i].Tag;

        // ZMIANA: Szukamy r�wnie� tag�w "ZadowolonyKlient" oraz "ZlyKlient", bo klient zmienia sw�j tag w trakcie reakcji!
        if (tag == "NormalCustomer" || tag.find("HelperCustomer") != std::string::npos ||
            tag == "ZadowolonyKlient" || tag == "ZlyKlient")
        {
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

            if (!custScript || (custScript->IsServed && custScript->State != CustomerState::LeavingReaction)) continue;

            glm::vec3 headPos = tf->GetPosition() + glm::vec3(0.0f, 3.0f, 0.0f);
            glm::vec4 clipSpace = viewProj * glm::vec4(headPos, 1.0f);
            if (clipSpace.w == 0.0f) continue;
            glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;

            float screenX = gameX + (ndc.x + 1.0f) * 0.5f * gameWidth;
            float screenY = gameY + (1.0f - ndc.y) * 0.5f * gameHeight;

            std::shared_ptr<Texture> iconToDraw = nullptr;

            // Ustawianie odpowiedniej ikonki reakcji
            if (custScript->State == CustomerState::LeavingReaction) {
                iconToDraw = custScript->m_WasCorrect ? m_SmileFaceIcon : m_AngryFaceIcon;
            }
            else if (!custScript->OrderTaken) {
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

    if (m_BookCloudIcon)
        Renderer2D::DrawQuad(cloudPos, cloudSize, m_BookCloudIcon, { 1.0f, 1.0f, 1.0f, 0.95f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });

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
    if (m_RecipeBookPanel.IsOpen() || m_IsGamePaused) return;

    auto* tags = m_ActiveScene->GetWorld().GetComponentVector<TagComponent>();
    auto* transforms = m_ActiveScene->GetWorld().GetComponentVector<TransformComponent>();
    auto* scripts = m_ActiveScene->GetWorld().GetComponentVector<NativeScriptComponent>();
    if (!tags || !transforms || !scripts) return;

    auto* camera = m_ActiveScene->GetCamera();
    glm::mat4 view = camera->GetViewMatrix();
    float currentAspect = gameWidth / (gameHeight > 0.0f ? gameHeight : 1.0f);
    float orthoSize = 10.0f * (camera->Zoom / 45.0f);
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
                if (s.Name == "PackageScript") { packScript = (PackageScript*)s.Instance; break; }
            }
            if (!packScript) continue;

            std::string iconPath = GetUIIconPathForIngredient(packScript->getType());
            std::shared_ptr<Texture> iconToDraw = iconPath.empty() ? m_QuestionMarkIcon : AssetManager::GetTexture(iconPath);

            DrawHoverCloudUI({ screenX, screenY }, iconToDraw, packScript->getIngredientAmount(), baseScale);
        }
    }
}

void GameGuiLayer::DrawCrateHoverInfo(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt)
{
    if (!m_ActiveScene || !m_ActiveScene->GetCamera()) return;
    if (m_RecipeBookPanel.IsOpen() || m_IsGamePaused) return;

    auto* transforms = m_ActiveScene->GetWorld().GetComponentVector<TransformComponent>();
    auto* scripts = m_ActiveScene->GetWorld().GetComponentVector<NativeScriptComponent>();
    if (!transforms || !scripts) return;

    auto* camera = m_ActiveScene->GetCamera();
    glm::mat4 view = camera->GetViewMatrix();
    float currentAspect = gameWidth / (gameHeight > 0.0f ? gameHeight : 1.0f);
    float orthoSize = 10.0f * (camera->Zoom / 45.0f);
    glm::mat4 proj3D = glm::ortho(-currentAspect * orthoSize, currentAspect * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
    glm::mat4 viewProj = proj3D * view;
    glm::vec2 mousePos = Gui::GetMappedMousePos();
    float hoverRadiusSq = (55.0f * baseScale) * (55.0f * baseScale);

    for (size_t i = 0; i < scripts->dense.size(); ++i) {
        auto& nsc = scripts->dense[i];
        CrateScript* crateScript = nullptr;
        for (auto& s : nsc.Scripts) {
            if (s.Name == "CrateScript") { crateScript = (CrateScript*)s.Instance; break; }
        }

        if (!crateScript || crateScript->m_CrateIngredient == IngredientType::None || !crateScript->isMIsHovered()) continue;

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

        std::string iconPath = GetUIIconPathForIngredient(crateScript->m_CrateIngredient);
        std::shared_ptr<Texture> iconToDraw = iconPath.empty() ? m_QuestionMarkIcon : AssetManager::GetTexture(iconPath);


        int amount = GameManagerScript::s_Instance ? GameManagerScript::s_Instance->GetIngredientCount(crateScript->m_CrateIngredient) : 0;
        DrawHoverCloudUI({ screenX, screenY }, iconToDraw, amount, baseScale);
    }
}

void GameGuiLayer::DrawMushroomBubble(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale)
{
    if (!m_ShowMushroomBubble) return;
    if (!m_ActiveScene || !m_ActiveScene->GetCamera()) return;
    if (!m_CoinCloudIcon) return;
    if (m_RecipeBookPanel.IsOpen() || m_IsGamePaused) return;

    auto* camera = m_ActiveScene->GetCamera();
    glm::mat4 view = camera->GetViewMatrix();
    float currentAspect = gameWidth / (gameHeight > 0.0f ? gameHeight : 1.0f);
    float orthoSize = 10.0f * (camera->Zoom / 45.0f);
    glm::mat4 proj3D = glm::ortho(-currentAspect * orthoSize, currentAspect * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
    glm::mat4 viewProj = proj3D * view;

    glm::vec3 dymekOffset = glm::vec3(0.0f, 1.5f, 0.0f);
    glm::vec4 clipSpace = viewProj * glm::vec4(m_MushroomPos3D + dymekOffset, 1.0f);

    if (clipSpace.w <= 0.0f) return;
    glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;

    float screenX = gameX + (ndc.x + 1.0f) * 0.5f * gameWidth;
    float screenY = gameY + (1.0f - ndc.y) * 0.5f * gameHeight;

    screenX += 180.0f * baseScale;

    float cloudW = 250.0f * baseScale;
    float cloudAspect = (float)m_CoinCloudIcon->GetHeight() / (float)m_CoinCloudIcon->GetWidth();
    float cloudH = cloudW * cloudAspect;

    glm::vec2 cloudPos = { screenX - cloudW * 0.5f, screenY - cloudH };
    Renderer2D::DrawQuad(cloudPos, { cloudW, cloudH }, m_CoinCloudIcon, { 1.0f, 1.0f, 1.0f, 0.95f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });

    std::string line1 = "Choose one,";
    std::string line2 = "I get to keep the other";

    float textScale = 0.85f * baseScale;
    float line1W = Gui::MeasureTextWidth(line1, textScale);
    float line2W = Gui::MeasureTextWidth(line2, textScale);
    float textHeight = Gui::MeasureTextHeight("A", textScale);

    float textOffsetX = 0.0f * baseScale;
    float textOffsetY = -25.0f * baseScale;

    glm::vec2 line1Pos = {
            cloudPos.x + (cloudW - line1W) * 0.5f + textOffsetX,
            cloudPos.y + (cloudH * 0.5f) + textOffsetY
    };

    glm::vec2 line2Pos = {
            cloudPos.x + (cloudW - line2W) * 0.5f + textOffsetX,
            line1Pos.y + textHeight * 1.3f
    };

    Gui::DrawGuiText(line1, line1Pos, textScale, titleColor);
    Gui::DrawGuiText(line2, line2Pos, textScale, titleColor);
}

void GameGuiLayer::DrawMachineWarningInfo(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt)
{
    if (m_MachineWarning.Timer <= 0.0f) return;
    if (m_MachineWarning.MachineEnt.id == std::numeric_limits<std::size_t>::max()) return;
    if (!m_ActiveScene || !m_ActiveScene->GetCamera()) return;
    if (m_RecipeBookPanel.IsOpen() || m_IsGamePaused) return;

    m_MachineWarning.Timer -= dt;

    auto* transforms = m_ActiveScene->GetWorld().GetComponentVector<TransformComponent>();
    if (!transforms) return;

    auto* tf = transforms->Get(m_MachineWarning.MachineEnt);
    if (!tf) return;

    float finalHeightOffset = 1.5f;

    auto* collider = m_ActiveScene->GetWorld().GetComponent<BoxColliderComponent>(m_MachineWarning.MachineEnt);
    if (collider)
    {
        finalHeightOffset = collider->Size.y + 0.2f;

    }

    glm::vec3 machinePos = tf->GetPosition() + glm::vec3(0.0f, finalHeightOffset, 0.0f);

    auto* camera = m_ActiveScene->GetCamera();
    glm::mat4 view = camera->GetViewMatrix();
    float currentAspect = gameWidth / (gameHeight > 0.0f ? gameHeight : 1.0f);
    float orthoSize = 10.0f * (camera->Zoom / 45.0f);
    glm::mat4 proj3D = glm::ortho(-currentAspect * orthoSize, currentAspect * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
    glm::mat4 viewProj = proj3D * view;

    glm::vec4 clipSpace = viewProj * glm::vec4(machinePos, 1.0f);
    if (clipSpace.w <= 0.0f) return;
    glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;

    float screenX = gameX + (ndc.x + 1.0f) * 0.5f * gameWidth;
    float screenY = gameY + (1.0f - ndc.y) * 0.5f * gameHeight;

    float alpha = std::clamp(m_MachineWarning.Timer / 0.2f, 0.0f, 1.0f);

    float cloudW = 160.0f * baseScale; // Zmniejszone z 200.0f
    float cloudAspect = m_CoinCloudIcon ? ((float)m_CoinCloudIcon->GetHeight() / (float)m_CoinCloudIcon->GetWidth()) : 0.6f;
    float cloudH = cloudW * cloudAspect;

    glm::vec2 cloudPos = { screenX - cloudW * 0.5f, screenY - cloudH };

    if (m_CoinCloudIcon) {
        Renderer2D::DrawQuad(cloudPos, { cloudW, cloudH }, m_CoinCloudIcon, { 1.0f, 1.0f, 1.0f, 0.95f * alpha }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    }

    float textScale = 0.62f * baseScale;
    float line1W = Gui::MeasureTextWidth(m_MachineWarning.Line1, textScale);
    float line2W = Gui::MeasureTextWidth(m_MachineWarning.Line2, textScale);
    float textHeight = Gui::MeasureTextHeight("A", textScale);

    bool hasLine2 = !m_MachineWarning.Line2.empty();
    float lineSpacing = textHeight * 0.2f;
    float totalTextHeight = hasLine2 ? (textHeight * 2.0f + lineSpacing) : textHeight;

    float visualBellyCenterY = cloudPos.y + (cloudH * 0.44f);

    float tweakX = 0.0f * baseScale;
    float tweakY = 0.0f * baseScale;

    float startY = visualBellyCenterY - (totalTextHeight * 0.5f) + tweakY;

    glm::vec2 line1Pos = {
            cloudPos.x + (cloudW - line1W) * 0.5f + tweakX,
            startY
    };

    glm::vec2 line2Pos = {
            cloudPos.x + (cloudW - line2W) * 0.5f + tweakX,
            startY + textHeight + lineSpacing
    };

    glm::vec4 currentTextColor = titleColor;
    currentTextColor.a *= alpha;

    Gui::DrawGuiText(m_MachineWarning.Line1, line1Pos, textScale, currentTextColor);

    if (hasLine2) {
        Gui::DrawGuiText(m_MachineWarning.Line2, line2Pos, textScale, currentTextColor);
    }

}

void GameGuiLayer::DrawHelperHint(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale)
{
    if (!m_ActiveScene || !m_ActiveScene->GetCamera()) return;
    if (m_RecipeBookPanel.IsOpen() || m_IsGamePaused) return;

    auto* scripts = m_ActiveScene->GetWorld().GetComponentVector<NativeScriptComponent>();
    auto* transforms = m_ActiveScene->GetWorld().GetComponentVector<TransformComponent>();
    if (!scripts || !transforms) return;

    for (size_t i = 0; i < scripts->dense.size(); ++i) {
        auto& nsc = scripts->dense[i];
        HelperCustomerScript* helperScript = nullptr;

        for (auto& s : nsc.Scripts) {
            if (s.Name == "HelperCustomerScript") {
                helperScript = (HelperCustomerScript*)s.Instance;
                break;
            }
        }

        // Jeśli to pierwszy helper i nadal czeka na podniesienie z podłogi
        if (helperScript && helperScript->m_IsFirstHelperInstance && helperScript->m_IsWaitingForPickup) {
            Entity helperEnt = scripts->reverse[i];
            auto* tf = transforms->Get(helperEnt);
            if (!tf) continue;

            // Rzutowanie pozycji 3D nad głową Helpera na ekran 2D
            auto* camera = m_ActiveScene->GetCamera();
            glm::mat4 view = camera->GetViewMatrix();
            float currentAspect = gameWidth / (gameHeight > 0.0f ? gameHeight : 1.0f);
            float orthoSize = 10.0f * (camera->Zoom / 45.0f);
            glm::mat4 proj3D = glm::ortho(-currentAspect * orthoSize, currentAspect * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
            glm::mat4 viewProj = proj3D * view;

            glm::vec3 worldPos = tf->GetPosition() + glm::vec3(0.0f, 2.8f, 0.0f); // Wysokość nad głową
            glm::vec4 clipSpace = viewProj * glm::vec4(worldPos, 1.0f);
            if (clipSpace.w <= 0.0f) continue;
            glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;

            float screenX = gameX + (ndc.x + 1.0f) * 0.5f * gameWidth;
            float screenY = gameY + (1.0f - ndc.y) * 0.5f * gameHeight;

            std::string line1 = "This customer wants to help";
            std::string line2 = "in your kitchen, pick him";
            std::string line3 = "up and place near a machine!";

            float hintScale = 0.9f * baseScale;
            float timeNow = glfwGetTime();
            float floatOffset = std::sin(timeNow * 2.2f) * 5.0f * baseScale;

            float w1 = Gui::MeasureTextWidth(line1, hintScale);
            float w2 = Gui::MeasureTextWidth(line2, hintScale);
            float w3 = Gui::MeasureTextWidth(line3, hintScale);
            float maxW = std::max({ w1, w2, w3 });

            float textH = Gui::MeasureTextHeight("A", hintScale);
            float lineSpacing = textH * 1.3f;
            float totalH = textH * 3.0f + lineSpacing * 2.0f;

            glm::vec2 blockPos = {
                screenX - maxW * 0.5f,
                screenY - totalH * 0.5f + floatOffset
            };

            float line1X = blockPos.x + (maxW - w1) * 0.5f;
            float line2X = blockPos.x + (maxW - w2) * 0.5f;
            float line3X = blockPos.x + (maxW - w3) * 0.5f;

            glm::vec4 shadowColor = { 0.0f, 0.0f, 0.0f, 0.5f };
            glm::vec4 textColor = { 1.0f, 1.0f, 1.0f, 1.0f };

            Gui::DrawGuiText(line1, { line1X + 1.5f, blockPos.y + 1.5f }, hintScale, shadowColor);
            Gui::DrawGuiText(line1, { line1X, blockPos.y }, hintScale, textColor);

            Gui::DrawGuiText(line2, { line2X + 1.5f, blockPos.y + lineSpacing + 1.5f }, hintScale, shadowColor);
            Gui::DrawGuiText(line2, { line2X, blockPos.y + lineSpacing }, hintScale, textColor);

            Gui::DrawGuiText(line3, { line3X + 1.5f, blockPos.y + lineSpacing * 2.0f + 1.5f }, hintScale, shadowColor);
            Gui::DrawGuiText(line3, { line3X, blockPos.y + lineSpacing * 2.0f }, hintScale, textColor);
        }
    }
}
void GameGuiLayer::DrawSpeedUpButton(float gameX, float gameY, float gameW, float gameH, float baseScale, float dt, bool isBlocked) {
    if (!m_SpeedUpIcon || m_SpeedUpIcon->GetRendererID() == 0) return;

    auto buildBtnTex = AssetManager::GetTexture("assets://UI/buildModeButton.png");
    float buildBtnHeight = 153.0f * baseScale;
    float buildAspect = buildBtnTex ? ((float)buildBtnTex->GetWidth() / (float)buildBtnTex->GetHeight()) : 2.5f;
    float buildBtnWidth = buildBtnHeight * buildAspect;

    float bookCloudH = 210.0f * baseScale * 1.3f;
    float buildBtnY = gameY + bookCloudH + 8.0f * baseScale;
    float buildBtnX = gameX + 35.0f * baseScale;
    float centerBtnX = buildBtnX + buildBtnWidth * 0.5f; 

    float iconHeight = 65.0f * baseScale;
    float iconAspect = (float)m_SpeedUpIcon->GetWidth() / (float)m_SpeedUpIcon->GetHeight();
    glm::vec2 baseSize = { iconHeight * iconAspect, iconHeight };

    glm::vec2 basePos = {
        centerBtnX - baseSize.x * 0.5f,
        buildBtnY + buildBtnHeight + 55.0f * baseScale
    };

    glm::vec2 mouse = Gui::GetMappedMousePos();
    bool inBounds = mouse.x >= basePos.x && mouse.x <= basePos.x + baseSize.x &&
        mouse.y >= basePos.y && mouse.y <= basePos.y + baseSize.y;

    static float s_scale = 1.0f;
    float targetScale = (inBounds && !isBlocked) ? 1.15f : 1.0f;
    s_scale += (targetScale - s_scale) * dt * 15.0f;

    bool isUIClicked = inBounds && !isBlocked && Input::IsMouseButtonPressed(0);
    bool isKeyboardPressed = Input::IsKeyPressed(GLFW_KEY_X);
    bool isHeld = isUIClicked || isKeyboardPressed;

    if (isUIClicked) {
        GameManagerScript::s_SpeedUpUIHeld = true;
    }

    glm::vec4 tint = (inBounds && !isBlocked) ? glm::vec4(0.85f, 0.85f, 0.85f, 1.0f) : glm::vec4(1.0f);

    if (isHeld) {
        tint *= glm::vec4(0.75f, 1.0f, 0.75f, 1.0f); 
    }

    glm::vec2 scaledSize = baseSize * s_scale;
    glm::vec2 scaledPos = {
        basePos.x + (baseSize.x - scaledSize.x) * 0.5f,
        basePos.y + (baseSize.y - scaledSize.y) * 0.5f
    };

    Renderer2D::DrawQuad(scaledPos, scaledSize, m_SpeedUpIcon, tint, { 0.0f, 1.0f }, { 1.0f, 0.0f });

    std::string label = "[X]"; 
    float textScale = 0.85f * baseScale * s_scale;
    float tw = Gui::MeasureTextWidth(label, textScale);

    glm::vec2 textPos = {
        basePos.x + (baseSize.x - tw) * 0.5f,
        basePos.y + baseSize.y + 8.0f * baseScale
    };

    glm::vec4 textShadowColor = { 0.0f, 0.0f, 0.0f, 0.6f };
    glm::vec4 textColor = isHeld ? glm::vec4(0.5f, 0.35f, 0.6f, 1.0f) : glm::vec4(157.0f / 255.0f, 113.0f / 255.0f, 180.0f / 255.0f, 1.0f);

    Gui::DrawGuiText(label, { textPos.x + 1.5f, textPos.y + 1.5f }, textScale, textShadowColor);
    Gui::DrawGuiText(label, textPos, textScale, textColor);

    if (inBounds && !isBlocked) {
        Input::SetUICaptureMouse(true);
    }
}