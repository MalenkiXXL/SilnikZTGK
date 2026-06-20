#include "MainMenuLayer.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Layers/GuiLayer/Utils/Gui.h"
#include "CookingStation/Layers/GuiLayer/Utils/Renderer2D.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Scene/SceneSerializer.h"
#include "CookingStation/Core/Application.h"
#include "CookingStation/Core/GraphicsSettings.h"
#include <glm/gtc/matrix_transform.hpp>
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Layers/GuiLayer/Panels/SettingsMenuPanel.h" 
#include "CookingStation/Layers/GuiLayer/Utils/AudioConfig.h"
#include <algorithm>
#include <string>

void MainMenuLayer::OnAttach()
{
    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = (float)windowSize.first;
    m_ViewportHeight = (float)windowSize.second;
    Gui::SetScreenSize(m_ViewportWidth, m_ViewportHeight);

    m_Background = std::make_shared<Texture>("assets://UI/menuImage.png");

    m_PlayBtnTex = std::make_shared<Texture>("assets://UI/playButton.png");
    m_SettingsBtnTex = std::make_shared<Texture>("assets://UI/settingsButton.png");
    m_CreditsBtnTex = std::make_shared<Texture>("assets://UI/creditsButton.png");
    m_ExitBtnTex = std::make_shared<Texture>("assets://UI/exitButton.png");
    m_BoardTex = std::make_shared<Texture>("assets://UI/cuttingBoard.png");

    m_SettingsPanel = std::make_shared<SettingsMenuPanel>();
    m_SettingsPanel->SetVisible(false);

    m_ShowMenuSubId = Application::Get().GetEventBus().Subscribe<ShowMainMenuEvent>(
        [this](const ShowMainMenuEvent&) {
            m_IsActive = true;
            m_SettingsPanel->SetVisible(false);
        }
    );
}

void MainMenuLayer::OnDetach()
{
    if (m_ShowMenuSubId != 0) {
        Application::Get().GetEventBus().Unsubscribe<ShowMainMenuEvent>(m_ShowMenuSubId);
        m_ShowMenuSubId = 0;
    }
}


bool MainMenuLayer::DrawScaledButton(const std::string& label,
    glm::vec2 basePos, glm::vec2 baseSize,
    float btnScale, float bsc,
    glm::vec4 colorNormal, glm::vec4 colorHover,
    bool hovered)
{
    return Gui::ScaledButton(label, basePos, baseSize, btnScale, bsc, colorNormal, colorHover, hovered);
}

bool MainMenuLayer::DrawImageButton(const std::shared_ptr<Texture>& tex, glm::vec2 basePos, glm::vec2 baseSize, float btnScale, float baseScale_, bool hovered)
{
    glm::vec2 scaledSize = baseSize * btnScale;
    glm::vec2 scaledPos = {
        basePos.x + (baseSize.x - scaledSize.x) * 0.5f,
        basePos.y + (baseSize.y - scaledSize.y) * 0.5f
    };

    glm::vec4 tint = hovered ? glm::vec4(0.85f, 0.85f, 0.85f, 1.0f) : glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    if (hovered && Input::IsMouseButtonPressed(0)) {
        tint = glm::vec4(0.65f, 0.65f, 0.65f, 1.0f);
    }

    if (tex && tex->GetRendererID() != 0) {
        Renderer2D::DrawQuad(scaledPos, scaledSize, tex, tint, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    }
    else {
        Gui::Panel(scaledPos, scaledSize, tint, 15.0f * baseScale_);
    }

    static std::unordered_map<void*, bool> lastHoverStates;
    void* btnId = tex.get();

    bool wasHoveredLastFrame = lastHoverStates[btnId];

    if (hovered && !wasHoveredLastFrame) {
        AudioEngine::Play(AudioConfig::ButtonHoverSound);
    }

    lastHoverStates[btnId] = hovered;

    bool isClicked = hovered && Input::IsMouseButtonJustPressed(0);
    if (isClicked) {
        AudioEngine::Play(AudioConfig::ButtonClickSound);
    }

    return isClicked;
}

void MainMenuLayer::OnUpdate(Timestep ts) {
    if (!m_IsActive) return;

    float dt = ts.GetSeconds();

    float baseScale = std::min(m_ViewportWidth / 1920.0f, m_ViewportHeight / 1080.0f);
    baseScale = std::max(baseScale, 0.35f);

    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    Renderer2D::BeginScene(uiProj);

    if (m_Background) {
        Renderer2D::DrawQuad({ 0.0f, 0.0f }, { m_ViewportWidth, m_ViewportHeight },
            m_Background, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    }
    else {
        Gui::Panel({ 0, 0 }, { m_ViewportWidth, m_ViewportHeight },
            { 0.08f, 0.08f, 0.12f, 1.0f }, 0.0f);
    }

    // Odwołujemy się do wskaźnika za pomocą ->
    if (m_SettingsPanel->IsVisible()) {
        m_SettingsPanel->OnUpdate(dt);
        m_SettingsPanel->Draw(baseScale);
    }
    else {
        DrawMainMenu(baseScale, dt);
    }

    Renderer2D::EndScene();
    glEnable(GL_DEPTH_TEST);
}

void MainMenuLayer::DrawMainMenu(float baseScale, float dt) {

    float settingsPadding = 30.0f * baseScale;
    float settingsTargetHeight = 85.0f * baseScale;

    auto getBtnSize = [&](const std::shared_ptr<Texture>& tex, float targetHeight) -> glm::vec2 {
        if (tex && tex->GetRendererID() != 0 && tex->GetHeight() > 0) {
            float aspect = (float)tex->GetWidth() / (float)tex->GetHeight();
            return { targetHeight * aspect, targetHeight };
        }
        return { targetHeight * 4.0f, targetHeight };
        };

    glm::vec2 settingsSize = getBtnSize(m_SettingsBtnTex, settingsTargetHeight);
    glm::vec2 settingsPos = { settingsPadding, settingsPadding };

    float mainBtnHeight = 130.0f * baseScale;
    float btnGap = 45.0f * baseScale;

    glm::vec2 playSize = getBtnSize(m_PlayBtnTex, mainBtnHeight);
    glm::vec2 creditsSize = getBtnSize(m_CreditsBtnTex, mainBtnHeight);
    glm::vec2 exitSize = getBtnSize(m_ExitBtnTex, mainBtnHeight);

    float totalH = (3.0f * mainBtnHeight) + (2.0f * btnGap);

    float boardHeight = 620.0f * baseScale;
    glm::vec2 boardSize = getBtnSize(m_BoardTex, boardHeight);

    float boardX = m_ViewportWidth * 0.07f;
    float boardY = m_ViewportHeight * 0.37f;

    if (m_BoardTex && m_BoardTex->GetRendererID() != 0) {
        Renderer2D::DrawQuad({ boardX, boardY }, boardSize, m_BoardTex, { 1.0f, 1.0f, 1.0f, 1.00f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    }
    else {
        Gui::Panel({ boardX, boardY }, boardSize, { 0.10f, 0.10f, 0.12f, 0.85f }, 20.0f * baseScale);
    }

    float startY = boardY + (boardSize.y - totalH) * 0.5f;

    glm::vec2 playPos = { boardX + (boardSize.x - playSize.x) * 0.5f, startY };
    glm::vec2 creditsPos = { boardX + (boardSize.x - creditsSize.x) * 0.5f, startY + mainBtnHeight + btnGap };
    glm::vec2 exitPos = { boardX + (boardSize.x - exitSize.x) * 0.5f, startY + 2.0f * (mainBtnHeight + btnGap) };

    glm::vec2 mouse = Gui::GetMappedMousePos();
    auto isHov = [&](glm::vec2 p, glm::vec2 s) {
        return mouse.x >= p.x && mouse.x <= p.x + s.x && mouse.y >= p.y && mouse.y <= p.y + s.y;
        };

    bool hoverSettings = isHov(settingsPos, settingsSize);
    bool hoverPlay = isHov(playPos, playSize);
    bool hoverCredits = isHov(creditsPos, creditsSize);
    bool hoverExit = isHov(exitPos, exitSize);

    float animSpeed = 14.0f;
    float lerpT = std::min(dt * animSpeed, 1.0f);

    m_SettingsBtnScale += ((hoverSettings ? 1.08f : 1.0f) - m_SettingsBtnScale) * lerpT;
    m_PlayBtnScale += ((hoverPlay ? 1.05f : 1.0f) - m_PlayBtnScale) * lerpT;
    m_CreditsBtnScale += ((hoverCredits ? 1.05f : 1.0f) - m_CreditsBtnScale) * lerpT;
    m_ExitBtnScale += ((hoverExit ? 1.05f : 1.0f) - m_ExitBtnScale) * lerpT;

    if (DrawImageButton(m_SettingsBtnTex, settingsPos, settingsSize, m_SettingsBtnScale, baseScale, hoverSettings)) {
        m_SettingsPanel->SetVisible(true);
        m_SettingsPanel->SyncWithEngine();
    }

    if (DrawImageButton(m_PlayBtnTex, playPos, playSize, m_PlayBtnScale, baseScale, hoverPlay)) {
        PlayGame();
    }

    if (DrawImageButton(m_CreditsBtnTex, creditsPos, creditsSize, m_CreditsBtnScale, baseScale, hoverCredits)) {
    }

    if (DrawImageButton(m_ExitBtnTex, exitPos, exitSize, m_ExitBtnScale, baseScale, hoverExit)) {
        Application::Get().GetWindow().ProcessWindowClose();
    }
}

void MainMenuLayer::PlayGame()
{
    m_IsActive = false;

    auto activeScene = SceneManager::NewScene();
    SceneSerializer serializer(activeScene.get());

    if (serializer.Deserialize("assets://levels/level02.json")) {
        activeScene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
        Gui::SetScreenSize(m_ViewportWidth, m_ViewportHeight);
        activeScene->SetState(SceneState::Play);
        activeScene->OnRuntimeStart();

        Application::Get().GetEventBus().Publish(GameStartedEvent{});

        Application::Get().GetEventBus().Publish(GameResumedEvent{});

        spdlog::info("Pomyslnie zaladowano level02.json!");
    }
    else {
        m_IsActive = true;
        spdlog::error("Blad: Nie udalo sie wczytac level02.json");
    }
}
void MainMenuLayer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
        return OnWindowResize(ev);
        });

    if (!m_IsActive) return;

    if (e.GetEventType() == EventType::MouseButtonPressed ||
        e.GetEventType() == EventType::MouseButtonReleased ||
        e.GetEventType() == EventType::MouseMoved ||
        e.GetEventType() == EventType::MouseScrolled)
    {
        e.Handled = true;
    }
}

bool MainMenuLayer::OnWindowResize(WindowResizeEvent& e) {
    m_ViewportWidth = (float)e.GetWidth();
    m_ViewportHeight = (float)e.GetHeight();
    Gui::SetScreenSize(m_ViewportWidth, m_ViewportHeight);
    return false;
}