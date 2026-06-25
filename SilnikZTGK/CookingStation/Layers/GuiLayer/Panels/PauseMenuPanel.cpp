#include "PauseMenuPanel.h"
#include "CookingStation/Layers/GuiLayer/Utils/Gui.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Core/Application.h"
#include "CookingStation/Events/KeyEvent.h"
#include "../Utils/GuiUtils.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Layers/GuiLayer/Utils/Renderer2D.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Layers/GuiLayer/Utils/AudioConfig.h"

PauseMenuPanel::PauseMenuPanel() {
    m_SettingsPanel = std::make_unique<SettingsMenuPanel>();

    auto& appBus = Application::Get().GetEventBus();

    appBus.Subscribe<GameStartedEvent>([this](const GameStartedEvent&) {
        m_IsPaused = false;
        });

    appBus.Subscribe<GamePausedEvent>([this](const GamePausedEvent&) {
        m_IsPaused = true;
        });

    appBus.Subscribe<GameResumedEvent>([this](const GameResumedEvent&) {
        m_IsPaused = false;
        });

    appBus.Subscribe<BuildModeToggledEvent>([this](const BuildModeToggledEvent& e) {
        m_IsBuildMode = e.IsActive;
        });
}

void PauseMenuPanel::TogglePause() {
    if (m_SettingsPanel->IsVisible()) {
        m_SettingsPanel->SetVisible(false);
    }
    else {
        if (!m_IsPaused) {
            Application::Get().GetEventBus().Publish(GamePausedEvent{});
            Application::Get().GetEventBus().Publish(PlayPauseSoundEvent{}); 
        }
        else {
            Application::Get().GetEventBus().Publish(GameResumedEvent{});
            Application::Get().GetEventBus().Publish(PlayUnpauseSoundEvent{}); 
        }
    }
}

void PauseMenuPanel::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
        if (ev.GetKeyCode() == GLFW_KEY_ESCAPE) {
            TogglePause();
            return true;
        }
        return false;
        });

    if (m_IsPaused) {
        if (e.GetEventType() == EventType::MouseButtonPressed ||
            e.GetEventType() == EventType::MouseMoved ||
            e.GetEventType() == EventType::MouseScrolled)
        {
            e.Handled = true;
        }
    }
}

void PauseMenuPanel::OnUpdate(float dt) {
    m_DeltaTime = dt;
    if (m_SettingsPanel->IsVisible()) {
        m_SettingsPanel->OnUpdate(dt);
    }
}

void PauseMenuPanel::Draw(float baseScale) {
    if (!m_IsPaused) return;

    auto windowSize = Input::GetWindowSize();

    Gui::Panel({ 0.0f, 0.0f }, { (float)windowSize.first, (float)windowSize.second }, { 0.05f, 0.05f, 0.05f, 0.75f }, 0.0f);

    if (m_SettingsPanel->IsVisible()) {
        m_SettingsPanel->Draw(baseScale);
        return;
    }

    auto boardTex = AssetManager::GetTexture("assets://UI/cuttingBoard.png");
    auto resumeTex = AssetManager::GetTexture("assets://UI/resumeButton.png");
    auto settingsTex = AssetManager::GetTexture("assets://UI/settingsButtonCarrot.png");
    auto menuTex = AssetManager::GetTexture("assets://UI/menuButton.png");
    auto pausedTextTex = AssetManager::GetTexture("assets://UI/pausedText.png");

    glm::vec2 uv0 = { 0.0f, 1.0f };
    glm::vec2 uv1 = { 1.0f, 0.0f };

    auto getAspectSize = [&](const std::shared_ptr<Texture>& tex, float targetHeight) -> glm::vec2 {
        if (tex && tex->GetRendererID() != 0) {
            float aspect = (float)tex->GetWidth() / (float)tex->GetHeight();
            return { targetHeight * aspect, targetHeight };
        }
        return { targetHeight * 3.0f, targetHeight };
        };

    float visualOffsetY = 60.0f * baseScale;


    float boardHeight = 600.0f * baseScale; 
    glm::vec2 boardSize = getAspectSize(boardTex, boardHeight);

    float boardX = (windowSize.first - boardSize.x) * 0.5f;
    float boardY = (windowSize.second - boardSize.y) * 0.5f + visualOffsetY;

    if (boardTex) {
        Renderer2D::DrawQuad({ boardX, boardY }, boardSize, boardTex->GetRendererID(), { 1.0f, 1.0f, 1.0f, 0.90f }, uv0, uv1);
    }
    else {
        Gui::Panel({ boardX, boardY }, boardSize, { 0.7f, 0.5f, 0.3f, 0.85f }, 20.0f);
    }

    if (pausedTextTex && pausedTextTex->GetRendererID() != 0) {
        float textAspect = (float)pausedTextTex->GetWidth() / (float)pausedTextTex->GetHeight();

        float targetTextWidth = boardSize.x * 0.8f;
        glm::vec2 textSize = { targetTextWidth, targetTextWidth / textAspect };

        float textGap = 50.0f * baseScale;

        float textX = (windowSize.first - textSize.x) * 0.5f;
        float textY = boardY - textSize.y - textGap;

        Renderer2D::DrawQuad({ textX, textY }, textSize, pausedTextTex->GetRendererID(), { 1.0f, 1.0f, 1.0f, 1.0f }, uv0, uv1);
    }

    float btnHeight = 115.0f * baseScale; 
    float btnGap = 45.0f * baseScale;     

    glm::vec2 playSize = getAspectSize(resumeTex, btnHeight);
    glm::vec2 settingsSize = getAspectSize(settingsTex, btnHeight);
    glm::vec2 exitSize = getAspectSize(menuTex, btnHeight);

    float totalH = (3.0f * btnHeight) + (2.0f * btnGap);
    float startY = (windowSize.second - totalH) * 0.5f + visualOffsetY; 

    glm::vec2 mouse = Gui::GetMappedMousePos();
    auto isHov = [&](glm::vec2 p, glm::vec2 s) {
        return mouse.x >= p.x && mouse.x <= p.x + s.x && mouse.y >= p.y && mouse.y <= p.y + s.y;
        };

    static bool s_LastMouseState = false;
    bool currentMouseState = Input::IsMouseButtonPressed(0);
    bool mouseClicked = currentMouseState && !s_LastMouseState;


    auto drawImageBtn = [&](auto tex, glm::vec2 basePos, glm::vec2 baseSize, float& scaleVar, bool hovered) {
        float targetScale = hovered ? 1.05f : 1.0f;
        scaleVar += (targetScale - scaleVar) * 15.0f * m_DeltaTime;

        glm::vec2 scaledSize = baseSize * scaleVar;
        glm::vec2 offset = (baseSize - scaledSize) * 0.5f;
        glm::vec2 finalPos = basePos + offset;

        glm::vec4 tint = hovered ? glm::vec4(0.85f, 0.85f, 0.85f, 1.0f) : glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        if (hovered && currentMouseState) {
            tint = glm::vec4(0.65f, 0.65f, 0.65f, 1.0f);
        }

        if (tex) {
            Renderer2D::DrawQuad(finalPos, scaledSize, tex->GetRendererID(), tint, uv0, uv1);
        }
        else {
            Gui::Panel(finalPos, scaledSize, tint, 10.0f);
        }

        static std::unordered_map<float*, bool> lastHoverStates;

        bool wasHoveredLastFrame = lastHoverStates[&scaleVar];

        if (hovered && !wasHoveredLastFrame) {
            AudioEngine::Play(AudioConfig::ButtonHoverSound);
        }

        lastHoverStates[&scaleVar] = hovered;

        bool isClicked = hovered && mouseClicked;
        if (isClicked) {
            AudioEngine::Play(AudioConfig::ButtonClickSound);
        }

        return isClicked;
        };

    float retX = (windowSize.first - playSize.x) * 0.5f;
    glm::vec2 retPos = { retX, startY };
    bool hoverRet = isHov(retPos, playSize);
    if (drawImageBtn(resumeTex, retPos, playSize, m_ResumeBtnScale, hoverRet)) {
        TogglePause();
    }

    float setX = (windowSize.first - settingsSize.x) * 0.5f;
    glm::vec2 setPos = { setX, startY + btnHeight + btnGap };
    bool hoverSet = isHov(setPos, settingsSize);
    if (drawImageBtn(settingsTex, setPos, settingsSize, m_SettingsBtnCarrotScale, hoverSet)) {
        m_SettingsPanel->SyncWithEngine();
        m_SettingsPanel->SetVisible(true);
    }

    float exitX = (windowSize.first - exitSize.x) * 0.5f;
    glm::vec2 exitPos = { exitX, startY + 2.0f * (btnHeight + btnGap) };
    bool hoverExit = isHov(exitPos, exitSize);
    if (drawImageBtn(menuTex, exitPos, exitSize, m_MenuBtnScale, hoverExit)) {
        m_IsPaused = false;
        SceneManager::NewScene();
        Application::Get().GetEventBus().Publish(ShowMainMenuEvent{});
    }

    s_LastMouseState = currentMouseState;
}