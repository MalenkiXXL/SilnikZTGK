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
#include <spdlog/spdlog.h>
#include <algorithm>
#include <string>

constexpr int MainMenuLayer::MsaaOptions[];

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

    auto& gs = GraphicsSettings::Get();
    for (int i = 0; i < GraphicsSettings::ResolutionCount; i++) {
        if (GraphicsSettings::Resolutions[i].first == gs.WindowWidth &&
            GraphicsSettings::Resolutions[i].second == gs.WindowHeight) {
            m_PendingResIndex = i;
            break;
        }
    }
    for (int i = 0; i < MsaaOptionCount; i++) {
        if (MsaaOptions[i] == gs.MsaaSamples) {
            m_PendingMsaaIndex = i;
            break;
        }
    }

    m_ShowMenuSubId = Application::Get().GetEventBus().Subscribe<ShowMainMenuEvent>(
        [this](const ShowMainMenuEvent&) {
            m_IsActive = true;
            m_SettingsOpen = false;
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

    return hovered && Input::IsMouseButtonJustPressed(0);
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

    if (m_SettingsOpen)
        DrawSettingsPanel(baseScale, dt);
    else
        DrawMainMenu(baseScale, dt);

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
        m_SettingsOpen = true;
        m_PendingMusicEnabled = AudioEngine::IsMusicEnabled();
        m_PendingSoundsEnabled = AudioEngine::AreSoundsEnabled();
    }

    if (DrawImageButton(m_PlayBtnTex, playPos, playSize, m_PlayBtnScale, baseScale, hoverPlay)) {
        PlayGame();
    }

    if (DrawImageButton(m_CreditsBtnTex, creditsPos, creditsSize, m_CreditsBtnScale, baseScale, hoverCredits)) {
    }

    if (DrawImageButton(m_ExitBtnTex, exitPos, exitSize, m_ExitBtnScale, baseScale, hoverExit)) {
        Application::Get().Close();
    }
}

void MainMenuLayer::DrawSettingsPanel(float baseScale, float dt)
{
    glm::vec2 mouse = Gui::GetMappedMousePos();

    // --- ŁADOWANIE TEKSTUR ---
    auto backBtnTex = AssetManager::GetTexture("assets://UI/backButton.png");
    auto applyBtnTex = AssetManager::GetTexture("assets://UI/applyButton.png");
    auto leftArrowTex = AssetManager::GetTexture("assets://UI/leftArrow.png");
    auto rightArrowTex = AssetManager::GetTexture("assets://UI/rightArrow.png");

    glm::vec2 uv0 = { 0.0f, 1.0f };
    glm::vec2 uv1 = { 1.0f, 0.0f };

    auto getAspectSize = [&](const std::shared_ptr<Texture>& tex, float targetHeight) -> glm::vec2 {
        if (tex && tex->GetRendererID() != 0) {
            float aspect = (float)tex->GetWidth() / (float)tex->GetHeight();
            return { targetHeight * aspect, targetHeight };
        }
        return { targetHeight * 3.0f, targetHeight };
        };

    // --- TŁO: DESKA DO KROJENIA ---
    float boardHeight = 750.0f * baseScale;
    glm::vec2 boardSize = getAspectSize(m_BoardTex, boardHeight);

    // Zabezpieczenie minimalnej szerokości, żeby elementy się zmieściły
    if (boardSize.x < 850.0f * baseScale) boardSize.x = 850.0f * baseScale;

    glm::vec2 panelPos = { (m_ViewportWidth - boardSize.x) * 0.5f, (m_ViewportHeight - boardSize.y) * 0.5f };

    if (m_BoardTex && m_BoardTex->GetRendererID() != 0) {
        Renderer2D::DrawQuad(panelPos, boardSize, m_BoardTex, { 1.0f, 1.0f, 1.0f, 1.0f }, uv0, uv1);
    }
    else {
        Gui::Panel(panelPos, boardSize, { 0.12f, 0.12f, 0.15f, 0.95f }, 20.0f * baseScale);
    }

    // --- TYTUŁ ---
    float titleScale = 1.6f * baseScale;
    float titleW = Gui::MeasureTextWidth("SETTINGS", titleScale);
    Gui::DrawGuiText("SETTINGS", { panelPos.x + (boardSize.x - titleW) * 0.5f, panelPos.y + 80.0f * baseScale }, titleScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    // --- LOGIKA MYSZKI I PRZYCISKÓW GRAFICZNYCH ---
    auto isHov = [&](glm::vec2 p, glm::vec2 s) {
        return mouse.x >= p.x && mouse.x <= p.x + s.x && mouse.y >= p.y && mouse.y <= p.y + s.y;
        };

    static bool s_LastMouseStateMainMenuSettings = false;
    bool currentMouseState = Input::IsMouseButtonPressed(0);
    bool mouseClicked = currentMouseState && !s_LastMouseStateMainMenuSettings;

    auto drawImageBtn = [&](std::shared_ptr<Texture> tex, glm::vec2 basePos, glm::vec2 baseSize, float& scaleVar, bool hovered) {
        float targetScale = hovered ? 1.05f : 1.0f;
        scaleVar += (targetScale - scaleVar) * 15.0f * dt;

        glm::vec2 scaledSize = baseSize * scaleVar;
        glm::vec2 offset = (baseSize - scaledSize) * 0.5f;
        glm::vec2 finalPos = basePos + offset;

        glm::vec4 tint = hovered ? glm::vec4(0.85f, 0.85f, 0.85f, 1.0f) : glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        if (hovered && currentMouseState) {
            tint = glm::vec4(0.65f, 0.65f, 0.65f, 1.0f);
        }

        if (tex && tex->GetRendererID() != 0) {
            Renderer2D::DrawQuad(finalPos, scaledSize, tex->GetRendererID(), tint, uv0, uv1);
        }
        else {
            Gui::Panel(finalPos, scaledSize, tint, 10.0f * baseScale);
        }

        return hovered && mouseClicked;
        };

    // --- POZYCJONOWANIE OPCJI USTAWIEŃ ---
    float startY = panelPos.y + 200.0f * baseScale;
    float rowGap = 100.0f * baseScale;

    float leftColX = panelPos.x + boardSize.x * 0.15f;
    float rightColX = panelPos.x + boardSize.x * 0.48f;
    float textScale = 1.0f * baseScale;

    float arrowYOffset = 25.0f * baseScale;
    float textYOffset = 18.0f * baseScale;

    glm::vec2 arrowSize = { 50.0f * baseScale, 50.0f * baseScale };
    float distanceBetweenArrows = 300.0f * baseScale;

    auto drawCenteredValue = [&](const std::string& text, float rowCenterY) {
        float textW = Gui::MeasureTextWidth(text, textScale);
        float centerX = rightColX + arrowSize.x + (distanceBetweenArrows - arrowSize.x) * 0.5f;
        Gui::DrawGuiText(text, { centerX - textW * 0.5f, rowCenterY - textYOffset }, textScale, { 1.0f, 1.0f, 1.0f, 1.0f });
        };

    // --- RZĄD 1: ROZDZIELCZOŚĆ ---
    float row1Y = startY;
    Gui::DrawGuiText("Resolution:", { leftColX, row1Y - textYOffset }, textScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    glm::vec2 resLeftPos = { rightColX, row1Y - arrowYOffset };
    bool hovResL = isHov(resLeftPos, arrowSize);
    if (drawImageBtn(leftArrowTex, resLeftPos, arrowSize, m_ResLeftBtnScale, hovResL)) {
        m_PendingResIndex = (m_PendingResIndex - 1 + GraphicsSettings::ResolutionCount) % GraphicsSettings::ResolutionCount;
    }

    std::string resText = std::to_string(GraphicsSettings::Resolutions[m_PendingResIndex].first) + " x " + std::to_string(GraphicsSettings::Resolutions[m_PendingResIndex].second);
    drawCenteredValue(resText, row1Y);

    glm::vec2 resRightPos = { rightColX + distanceBetweenArrows, row1Y - arrowYOffset };
    bool hovResR = isHov(resRightPos, arrowSize);
    if (drawImageBtn(rightArrowTex, resRightPos, arrowSize, m_ResRightBtnScale, hovResR)) {
        m_PendingResIndex = (m_PendingResIndex + 1) % GraphicsSettings::ResolutionCount;
    }

    // --- RZĄD 2: ANTI-ALIASING ---
    float row2Y = startY + rowGap;
    Gui::DrawGuiText("Antialiasing:", { leftColX, row2Y - textYOffset }, textScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    glm::vec2 msaaLeftPos = { rightColX, row2Y - arrowYOffset };
    bool hovMsaaL = isHov(msaaLeftPos, arrowSize);
    if (drawImageBtn(leftArrowTex, msaaLeftPos, arrowSize, m_MsaaLeftBtnScale, hovMsaaL)) {
        m_PendingMsaaIndex = (m_PendingMsaaIndex - 1 + MsaaOptionCount) % MsaaOptionCount;
    }

    int msaaSamples = MsaaOptions[m_PendingMsaaIndex];
    std::string msaaText = (msaaSamples == 1) ? "Off" : "MSAA x" + std::to_string(msaaSamples);
    drawCenteredValue(msaaText, row2Y);

    glm::vec2 msaaRightPos = { rightColX + distanceBetweenArrows, row2Y - arrowYOffset };
    bool hovMsaaR = isHov(msaaRightPos, arrowSize);
    if (drawImageBtn(rightArrowTex, msaaRightPos, arrowSize, m_MsaaRightBtnScale, hovMsaaR)) {
        m_PendingMsaaIndex = (m_PendingMsaaIndex + 1) % MsaaOptionCount;
    }

    // --- RZĄD 3: MUZYKA (Tło) ---
    float row3Y = row2Y + rowGap;
    Gui::DrawGuiText("Music:", { leftColX, row3Y - textYOffset }, textScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    glm::vec2 musLeftPos = { rightColX, row3Y - arrowYOffset };
    bool hovMusL = isHov(musLeftPos, arrowSize);
    if (drawImageBtn(leftArrowTex, musLeftPos, arrowSize, m_MusicLeftBtnScale, hovMusL)) {
        m_PendingMusicEnabled = !m_PendingMusicEnabled;
    }

    std::string musText = m_PendingMusicEnabled ? "ON" : "OFF";
    drawCenteredValue(musText, row3Y);

    glm::vec2 musRightPos = { rightColX + distanceBetweenArrows, row3Y - arrowYOffset };
    bool hovMusR = isHov(musRightPos, arrowSize);
    if (drawImageBtn(rightArrowTex, musRightPos, arrowSize, m_MusicRightBtnScale, hovMusR)) {
        m_PendingMusicEnabled = !m_PendingMusicEnabled;
    }

    // --- RZĄD 4: DŹWIĘKI (SFX) ---
    float row4Y = row3Y + rowGap;
    Gui::DrawGuiText("Sounds:", { leftColX, row4Y - textYOffset }, textScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    glm::vec2 sndLeftPos = { rightColX, row4Y - arrowYOffset };
    bool hovSndL = isHov(sndLeftPos, arrowSize);
    if (drawImageBtn(leftArrowTex, sndLeftPos, arrowSize, m_SoundsLeftBtnScale, hovSndL)) {
        m_PendingSoundsEnabled = !m_PendingSoundsEnabled;
    }

    std::string sndText = m_PendingSoundsEnabled ? "ON" : "OFF";
    drawCenteredValue(sndText, row4Y);

    glm::vec2 sndRightPos = { rightColX + distanceBetweenArrows, row4Y - arrowYOffset };
    bool hovSndR = isHov(sndRightPos, arrowSize);
    if (drawImageBtn(rightArrowTex, sndRightPos, arrowSize, m_SoundsRightBtnScale, hovSndR)) {
        m_PendingSoundsEnabled = !m_PendingSoundsEnabled;
    }

    // --- DOLNY PANEL (PRZYCISKI BACK / APPLY) ---
    float btnHeight = 90.0f * baseScale;
    glm::vec2 backBtnSize = getAspectSize(backBtnTex, btnHeight);
    glm::vec2 applyBtnSize = getAspectSize(applyBtnTex, btnHeight);

    float bottomY = panelPos.y + boardSize.y - btnHeight - 70.0f * baseScale;

    // Przycisk BACK
    glm::vec2 backPos = { panelPos.x + boardSize.x * 0.12f, bottomY };
    bool hovBack = isHov(backPos, backBtnSize);
    if (drawImageBtn(backBtnTex, backPos, backBtnSize, m_BackBtnScale, hovBack)) {
        m_SettingsOpen = false;
    }

    // Przycisk APPLY
    glm::vec2 applyPos = { panelPos.x + boardSize.x * 0.88f - applyBtnSize.x, bottomY };
    bool hovApply = isHov(applyPos, applyBtnSize);
    if (drawImageBtn(applyBtnTex, applyPos, applyBtnSize, m_ApplyBtnScale, hovApply)) {

        Application::Get().GetEventBus().Publish(AudioSettingsChangedEvent{
            m_PendingMusicEnabled,
            m_PendingSoundsEnabled
            });

        auto& gs = GraphicsSettings::Get();
        int newWidth = GraphicsSettings::Resolutions[m_PendingResIndex].first;
        int newHeight = GraphicsSettings::Resolutions[m_PendingResIndex].second;
        int newMsaa = MsaaOptions[m_PendingMsaaIndex];

        if (gs.WindowWidth != newWidth || gs.WindowHeight != newHeight || gs.MsaaSamples != newMsaa) {
            gs.WindowWidth = newWidth;
            gs.WindowHeight = newHeight;
            gs.MsaaSamples = newMsaa;

            Application::Get().ApplyGraphicsSettings();
        }

        m_ViewportWidth = (float)gs.WindowWidth;
        m_ViewportHeight = (float)gs.WindowHeight;

        m_SettingsOpen = false;
    }

    s_LastMouseStateMainMenuSettings = currentMouseState;
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