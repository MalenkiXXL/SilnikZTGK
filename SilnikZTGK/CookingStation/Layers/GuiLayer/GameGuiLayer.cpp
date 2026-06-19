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
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Core/Physics.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath> 

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

    m_BuildModePanel.Init(m_CoinIcon);

    m_IngredientsCarousel.Init(true);
    m_MachinesCarousel.Init(false);

    m_GameStartedSubId = Application::Get().GetEventBus().Subscribe<GameStartedEvent>(
        [this](const GameStartedEvent&) {
            if (m_ActiveScene) {
                auto& oldBus = m_ActiveScene->GetWorld().GetEventBus();
                if (m_InventorySubId != 0) { oldBus.Unsubscribe<InventoryChangedEvent>(m_InventorySubId); m_InventorySubId = 0; }
                if (m_MoneySubId != 0) { oldBus.Unsubscribe<MoneyChangedEvent>(m_MoneySubId);         m_MoneySubId = 0; }
                if (m_OrderTakenSubId != 0) { oldBus.Unsubscribe<OrderTakenEvent>(m_OrderTakenSubId);      m_OrderTakenSubId = 0; }
            }

            m_ActiveScene = SceneManager::GetActiveScene();
            m_ActiveOrderTickets.clear();
            m_LastMoney = -1;

            // NOWOŒÆ: Twardy reset dla panelu budowania, kiedy w³¹czasz now¹ grê (¿eby zmienna nie pamiêta³a dawnego wyboru!)
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
            m_IsGamePaused = false;
        }
    );

    auto& appBus = Application::Get().GetEventBus();
    appBus.Subscribe<GamePausedEvent>([this](const GamePausedEvent&) { m_IsGamePaused = true; });
    appBus.Subscribe<GameResumedEvent>([this](const GameResumedEvent&) { m_IsGamePaused = false; });

    appBus.Subscribe<BuildModeToggledEvent>([this](const BuildModeToggledEvent& e) {
        if (!e.IsActive) m_BuildModePanel.Deactivate();
        else             m_BuildModePanel.Activate();
        });

    // NOWOŒÆ: Kiedy wychodzisz do menu g³ównego z poziomu zapauzowanej gry, automatycznie wy³¹cza Build Mode
    appBus.Subscribe<ShowMainMenuEvent>([this](const ShowMainMenuEvent&) {
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
    }

    if (m_GameStartedSubId != 0)
        Application::Get().GetEventBus().Unsubscribe<GameStartedEvent>(m_GameStartedSubId);
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

    if (isWarning) {
        if (m_CoinCloudIcon)
            Renderer2D::DrawQuad(cloudPos, cloudSize, m_CoinCloudIcon, { 0.95f, 0.85f, 0.85f, 0.95f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
        Renderer2D::DrawQuad(coinPos, coinSize, iconTex, { 0.75f, 0.35f, 0.35f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    }
    else {
        BubblyUI::DrawBubblyImage(m_BubblyStates, "CloudIcon", m_CoinCloudIcon, cloudPos, cloudSize, dt, false, 1.05f, false);
        BubblyUI::DrawBubblyImage(m_BubblyStates, "CoinIcon", iconTex, coinPos, coinSize, dt, false, 1.05f, false);
    }

    float textDrawX = textPos.x + shakeOffsetX;
    float textDrawY = coinPos.y + (coinSize.y * 0.5f) - baselineOffset + (textHeight * 0.25f);

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

void GameGuiLayer::DrawOrderTickets(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale) {
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
        float ticketHeight = isFirst ? (220.0f * baseScale) : (140.0f * baseScale);
        bool isHelper = (tagComp->Tag == "HelperCustomer");

        std::shared_ptr<Texture> ticketTex = isHelper ? m_HelperOrderTex : m_CustomerOrderTex;
        if (!ticketTex) ticketTex = m_BookCloudIcon;

        if (ticketTex) {
            glm::vec2 ticketSize = GuiUtils::CalculateAspectSize(ticketTex, ticketHeight);
            glm::vec2 ticketPos = { gameX + gameWidth - ticketSize.x - rightMargin, currentY };

            Renderer2D::DrawQuad(ticketPos, ticketSize, ticketTex, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });

            if (custScript) {
                std::shared_ptr<Texture> iconToDraw = nullptr;

                switch (custScript->WantedIngredient) {
                case IngredientType::Tomato: iconToDraw = m_TomatoIcon; break;
                case IngredientType::Cheese: iconToDraw = m_CheeseIcon; break;
                case IngredientType::Ham:    iconToDraw = m_HamIcon;    break;
                case IngredientType::Milk:   iconToDraw = m_MilkIcon;   break;
                case IngredientType::Flour:  iconToDraw = m_FlourIcon;  break;
                case IngredientType::Sandwich:   iconToDraw = AssetManager::GetTexture("assets://UI/sandwich.png"); break;
                default: iconToDraw = m_QuestionMarkIcon; break;
                }

                if (iconToDraw) {
                    float iconH = isFirst ? (70.0f * baseScale) : (40.0f * baseScale);
                    glm::vec2 iconSize = GuiUtils::CalculateAspectSize(iconToDraw, iconH);

                    glm::vec2 iconPos = {
                        ticketPos.x + (ticketSize.x - iconSize.x) * 0.5f,
                        ticketPos.y + (ticketSize.y - iconSize.y) * 0.40f
                    };

                    Renderer2D::DrawQuad(iconPos, iconSize, iconToDraw, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });

                    if (!isHelper) {
                        std::string rewardText = std::to_string((int)custScript->OrderPrice);
                        float textScale = isFirst ? (0.75f * baseScale) : (0.55f * baseScale);
                        float textWidth = Gui::MeasureTextWidth(rewardText, textScale);

                        float textYOffset = isFirst ? (18.0f * baseScale) : (12.0f * baseScale);
                        glm::vec2 textPos = {
                            ticketPos.x + (ticketSize.x - textWidth) * 0.5f,
                            ticketPos.y + ticketSize.y - textYOffset * 2.75f
                        };

                        Gui::DrawGuiText(rewardText, { textPos.x + 2.0f, textPos.y + 2.0f }, textScale, { 0.1f, 0.2f, 0.1f, 0.7f });
                        Gui::DrawGuiText(rewardText, textPos, textScale, { 1.0f, 1.0f, 1.0f, 1.0f });
                    }
                }
            }

            currentY += ticketHeight + (10.0f * baseScale);
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

    Gui::BeginFrame();
    Gui::UpdateDeltaTime(ts.GetSeconds());
    float dt = ts.GetSeconds();

    m_ActiveScene = SceneManager::GetActiveScene();

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
        Renderer2D::BeginScene(uiProj);
        m_BuildModePanel.DrawOverlay(m_ViewportWidth, m_ViewportHeight, baseScale);
        Renderer2D::EndScene();

        glDisable(GL_DEPTH_TEST);
    }

    Renderer2D::BeginScene(uiProj);

    bool isBookOpen = m_RecipeBookPanel.IsOpen();
    bool isPausedBlocked = m_IsGamePaused && !m_BuildModePanel.IsActive();
    bool isPlayMode = !m_IsGamePaused && !m_BuildModePanel.IsActive();

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
                        if (s.Instance) {
                            s.Instance->OnHoverCursor();
                        }
                    }
                }
            }
        }
    }

    DrawCrateHoverInfo(gameX, gameY, gameWidth, gameHeight, baseScale, dt);

    if (!GameManagerScript::s_IsTutorialMode)
    {
        DrawQuestPanel(gameX, gameY, gameWidth, gameHeight, baseScale, isPlayMode);
        DrawIngredientClouds(gameX, gameY, gameWidth, gameHeight, baseScale, dt);
        m_RecipeBookPanel.Draw(gameX, gameY, gameWidth, gameHeight, baseScale, dt, m_IsGamePaused);
        DrawCustomerOrders(gameX, gameY, gameWidth, gameHeight, baseScale);
        DrawOrderTickets(gameX, gameY, gameWidth, gameHeight, baseScale);
        DrawPackageHoverInfo(gameX, gameY, gameWidth, gameHeight, baseScale, dt);
        m_BuildModePanel.DrawButton(gameX, gameY, gameWidth, gameHeight, baseScale, dt, isPausedBlocked || isBookOpen);
        m_BuildModePanel.DrawPanel(gameX, gameY, gameWidth, gameHeight, baseScale, dt);

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

        float speakerScale = 2.0f * baseScale;
        float speakerWidth = Gui::MeasureTextWidth(speaker, speakerScale);
        float speakerX = gameX + (gameWidth - speakerWidth) * 0.5f;

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
            float lineStartX = gameX + (gameWidth - totalLineWidth) * 0.5f;

            Gui::DrawGuiText(drawnLine, { lineStartX + 2.0f, currentY + 2.0f }, textScale, { 0.0f, 0.0f, 0.0f, 0.5f });
            Gui::DrawGuiText(drawnLine, { lineStartX, currentY }, textScale, glm::vec4(1.0f));

            // Rejestrujemy najwiêksz¹ szerokoœæ!
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

                float blockRightEdge = gameX + (gameWidth + maxActualWidth) * 0.5f;
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

    if (m_BuildModePanel.IsActive()) m_BuildModePanel.UpdatePlacement(m_ActiveScene);

    if (m_PausePanel && m_PausePanel->IsPaused()) {
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        Renderer2D::BeginScene(uiProj);
        m_PausePanel->OnUpdate(dt);
        m_PausePanel->Draw(baseScale);
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

    dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& ev) {
        m_IngredientsCarousel.OnMouseScrolled(ev, m_ViewportWidth, 8);
        m_MachinesCarousel.OnMouseScrolled(ev, m_ViewportWidth, 8);
        return false;
        });

    dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
        if (ev.GetKeyCode() == 292 && ev.GetRepeatCode() == 0) m_ShowFPS = !m_ShowFPS;
        if (ev.GetKeyCode() == 258 && ev.GetRepeatCode() == 0) {
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
    float hoverRadius = (state == QuestEventState::WaitingForAccept ? 150.0f : 100.0f) * baseScale;
    float dx = mousePos.x - boothScreenX;
    float dy = mousePos.y - (boothScreenY + 30.0f * baseScale);
    bool isHovering3D = ((dx * dx + dy * dy) <= (hoverRadius * hoverRadius));

    float margin = 30.0f * baseScale;
    bool isHoveringPanel = (mousePos.x >= cloudPos.x - margin && mousePos.x <= cloudPos.x + cloudSize.x + margin &&
        mousePos.y >= cloudPos.y - margin && mousePos.y <= cloudPos.y + cloudSize.y + margin);

    if (!isHovering3D && !isHoveringPanel) return;
    Input::SetUICaptureMouse(true);

    if (state == QuestEventState::QuestActive)
    {
        static std::shared_ptr<Texture> s_EventCloudTex = AssetManager::GetTexture("assets://UI/Events/EventCloud.png");
        static std::shared_ptr<Texture> s_EventRewardTex = AssetManager::GetTexture("assets://UI/Events/EventReward.png");
        if (!s_EventCloudTex) return;

        float cloudW = 300.0f * baseScale;
        float cloudAspect = (float)s_EventCloudTex->GetHeight() / (float)s_EventCloudTex->GetWidth();
        float cloudH = cloudW * cloudAspect;

        glm::vec2 newCloudPos = { boothScreenX - cloudW * 0.5f, boothScreenY - cloudH * 1.2f };
        if (newCloudPos.x < gameX + 10.0f) newCloudPos.x = gameX + 10.0f;
        if (newCloudPos.x + cloudW > gameX + gameWidth - 10.0f) newCloudPos.x = gameX + gameWidth - cloudW - 10.0f;
        if (newCloudPos.y < gameY + 10.0f) newCloudPos.y = gameY + 10.0f;

        Renderer2D::DrawQuad(newCloudPos, { cloudW, cloudH }, s_EventCloudTex, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });

        glm::vec4 titleColor = { 0.42f, 0.24f, 0.46f, 1.0f };
        glm::vec4 descColor = { 0.56f, 0.37f, 0.66f, 1.0f };
        glm::vec4 circleColor = { 0.58f, 0.35f, 0.65f, 1.0f };

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

        if (s_EventRewardTex) {
            float rewardAspect = (float)s_EventRewardTex->GetWidth() / (float)s_EventRewardTex->GetHeight();
            float rewardH = cloudH * 0.6f;
            float rewardW = rewardH * rewardAspect;

            glm::vec2 rewardPos = { newCloudPos.x + cloudW - rewardW * 2.3f, newCloudPos.y - rewardH * 0.6f };

            Renderer2D::DrawQuad(rewardPos, { rewardW, rewardH }, s_EventRewardTex, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });

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
    float bakedCoinCenterX = newCloudPos.x + cloudW * 0.40f;
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

    if (acceptHit && Input::IsMouseButtonJustPressed(0))
        GameManagerScript::s_Instance->AcceptQuest();

    if (skipHit && skipsLeft > 0 && Input::IsMouseButtonJustPressed(0))
        GameManagerScript::s_Instance->SkipQuest();
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
            if (!custScript->OrderTaken) iconToDraw = m_QuestionMarkIcon;

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