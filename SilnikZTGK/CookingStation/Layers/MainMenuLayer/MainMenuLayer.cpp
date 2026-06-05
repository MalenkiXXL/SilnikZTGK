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

    // Synchronizuj indeksy pending z aktualnie zastosowanymi ustawieniami
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


// Przycisk zwykły
bool MainMenuLayer::DrawScaledButton(const std::string& label,
    glm::vec2 basePos, glm::vec2 baseSize,
    float btnScale, float bsc,
    glm::vec4 colorNormal, glm::vec4 colorHover,
    bool hovered)
{
    return Gui::ScaledButton(label, basePos, baseSize, btnScale, bsc, colorNormal, colorHover, hovered);
}

// Metoda do przycisków graficznych
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
    float baseScale = std::max(m_ViewportHeight / 1080.0f, 0.5f);

    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    Renderer2D::BeginScene(uiProj);

    // Tlo
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

    // Zębatka która otwiera ustawienia
    float settingsPadding = 30.0f * baseScale;
    float settingsTargetHeight = 85.0f * baseScale; 

    // Obliczenie rozmiaru z zachowaniem proporcji
    auto getBtnSize = [&](const std::shared_ptr<Texture>& tex, float targetHeight) -> glm::vec2 {
        if (tex && tex->GetRendererID() != 0) {
            float aspect = (float)tex->GetWidth() / (float)tex->GetHeight();
            return { targetHeight * aspect, targetHeight };
        }
        return { targetHeight * 4.0f, targetHeight }; // Fallback w razie braku tekstury
        };

    glm::vec2 settingsSize = getBtnSize(m_SettingsBtnTex, settingsTargetHeight);
    glm::vec2 settingsPos = { settingsPadding, settingsPadding };

    //Główne przyciski
    float mainBtnHeight = 120.0f * baseScale;
    float btnGap = 60.0f * baseScale;
    float blockLeft = m_ViewportWidth * 0.10f;
    float blockTop = m_ViewportHeight * 0.435f;

    glm::vec2 playSize = getBtnSize(m_PlayBtnTex, mainBtnHeight);
    glm::vec2 creditsSize = getBtnSize(m_CreditsBtnTex, mainBtnHeight);
    glm::vec2 exitSize = getBtnSize(m_ExitBtnTex, mainBtnHeight);

    glm::vec2 playPos = { blockLeft, blockTop };
    glm::vec2 creditsPos = { blockLeft, blockTop + (mainBtnHeight + btnGap) };
    glm::vec2 exitPos = { blockLeft, blockTop + (mainBtnHeight + btnGap) * 2.0f };

    // Hover i animacje
    glm::vec2 mouse = Gui::GetMappedMousePos();
    auto isHov = [&](glm::vec2 p, glm::vec2 s) {
        return mouse.x >= p.x && mouse.x <= p.x + s.x &&
            mouse.y >= p.y && mouse.y <= p.y + s.y;
        };

    bool hoverSettings = isHov(settingsPos, settingsSize);
    bool hoverPlay = isHov(playPos, playSize);
    bool hoverCredits = isHov(creditsPos, creditsSize);
    bool hoverExit = isHov(exitPos, exitSize);

    float animSpeed = 14.0f; // Minimalnie przyspieszyłem animację sprężystości
    m_SettingsBtnScale += ((hoverSettings ? 1.08f : 1.0f) - m_SettingsBtnScale) * dt * animSpeed;
    m_PlayBtnScale += ((hoverPlay ? 1.05f : 1.0f) - m_PlayBtnScale) * dt * animSpeed;
    m_CreditsBtnScale += ((hoverCredits ? 1.05f : 1.0f) - m_CreditsBtnScale) * dt * animSpeed;
    m_ExitBtnScale += ((hoverExit ? 1.05f : 1.0f) - m_ExitBtnScale) * dt * animSpeed;

    // Zębatka na samej górze
    if (DrawImageButton(m_SettingsBtnTex, settingsPos, settingsSize, m_SettingsBtnScale, baseScale, hoverSettings)) {
        m_SettingsOpen = true;
    }

    // Blok menu 
    if (DrawImageButton(m_PlayBtnTex, playPos, playSize, m_PlayBtnScale, baseScale, hoverPlay)) {
        PlayGame();
    }

    if (DrawImageButton(m_CreditsBtnTex, creditsPos, creditsSize, m_CreditsBtnScale, baseScale, hoverCredits)) {
        // Logika creditsów
    }

    if (DrawImageButton(m_ExitBtnTex, exitPos, exitSize, m_ExitBtnScale, baseScale, hoverExit)) {
        // Logika exit
    }
}

void MainMenuLayer::DrawSettingsPanel(float baseScale, float dt)
{
    glm::vec2 mouse = Gui::GetMappedMousePos();

    float panelW = 700.0f * baseScale;
    float panelH = 500.0f * baseScale;
    float panelX = (m_ViewportWidth - panelW) * 0.5f;
    float panelY = (m_ViewportHeight - panelH) * 0.5f;

    // Ramka + tło
    Gui::Panel({ panelX - 3.0f, panelY - 3.0f }, { panelW + 6.0f, panelH + 6.0f },
        { 0.6f, 0.6f, 0.7f, 0.5f }, 20.0f * baseScale);
    Gui::Panel({ panelX, panelY }, { panelW, panelH },
        { 0.10f, 0.10f, 0.14f, 0.97f }, 18.0f * baseScale);

    // Nagłówek
    float       titleScale = 1.6f * baseScale;
    std::string title = "SETTINGS";
    float       titleW = Gui::MeasureTextWidth(title, titleScale);
    Gui::DrawGuiText(title,
        { panelX + (panelW - titleW) * 0.5f + 2.0f, panelY + 38.0f * baseScale + 2.0f },
        titleScale, { 0.0f, 0.0f, 0.0f, 0.6f });
    Gui::DrawGuiText(title,
        { panelX + (panelW - titleW) * 0.5f, panelY + 38.0f * baseScale },
        titleScale, { 0.9f, 0.9f, 1.0f, 1.0f });

    float rowH = 80.0f * baseScale;
    float rowStart = panelY + 110.0f * baseScale;
    float labelX = panelX + 40.0f * baseScale;
    float arrowW = 48.0f * baseScale;
    float arrowH = 48.0f * baseScale;
    float valueBoxW = 260.0f * baseScale;
    float valueBoxH = 52.0f * baseScale;
    float controlsX = panelX + panelW * 0.45f;
    float labelScale = 0.9f * baseScale;
    float animSpeed = 14.0f;

    // Rozdzielczość
    {
        float      rowY = rowStart;
        std::string lbl = "Resolution";
        float lblH = Gui::MeasureTextHeight(lbl, labelScale);
        float baselineOff = 32.0f * 0.8f * labelScale;
        Gui::DrawGuiText(lbl,
            { labelX, rowY + (arrowH - lblH) * 0.5f - baselineOff + lblH * 0.5f },
            labelScale, { 0.85f, 0.85f, 0.90f, 1.0f });

        glm::vec2 leftPos = { controlsX, rowY };
        glm::vec2 rightPos = { controlsX + arrowW + valueBoxW + 8.0f * baseScale, rowY };
        glm::vec2 arrowSize = { arrowW, arrowH };

        bool hovL = mouse.x >= leftPos.x && mouse.x <= leftPos.x + arrowW &&
            mouse.y >= leftPos.y && mouse.y <= leftPos.y + arrowH;
        bool hovR = mouse.x >= rightPos.x && mouse.x <= rightPos.x + arrowW &&
            mouse.y >= rightPos.y && mouse.y <= rightPos.y + arrowH;

        m_ResLeftBtnScale += ((hovL ? 1.08f : 1.0f) - m_ResLeftBtnScale) * dt * animSpeed;
        m_ResRightBtnScale += ((hovR ? 1.08f : 1.0f) - m_ResRightBtnScale) * dt * animSpeed;

        if (DrawScaledButton("<", leftPos, arrowSize, m_ResLeftBtnScale, baseScale,
            { 0.25f, 0.25f, 0.30f, 1.0f }, { 0.40f, 0.40f, 0.50f, 1.0f }, hovL))
        {
            m_PendingResIndex = (m_PendingResIndex - 1 + GraphicsSettings::ResolutionCount)
                % GraphicsSettings::ResolutionCount;
        }
        if (DrawScaledButton(">", rightPos, arrowSize, m_ResRightBtnScale, baseScale,
            { 0.25f, 0.25f, 0.30f, 1.0f }, { 0.40f, 0.40f, 0.50f, 1.0f }, hovR))
        {
            m_PendingResIndex = (m_PendingResIndex + 1) % GraphicsSettings::ResolutionCount;
        }

        glm::vec2 vbPos = { controlsX + arrowW + 8.0f * baseScale,
                            rowY + (arrowH - valueBoxH) * 0.5f };
        Gui::Panel(vbPos, { valueBoxW, valueBoxH }, { 0.18f, 0.18f, 0.22f, 1.0f }, 10.0f * baseScale);

        auto [rw, rh] = GraphicsSettings::Resolutions[m_PendingResIndex];
        std::string resStr = std::to_string(rw) + " x " + std::to_string(rh);
        float valScale = 0.85f * baseScale;
        float resW = Gui::MeasureTextWidth(resStr, valScale);
        float resH = Gui::MeasureTextHeight(resStr, valScale);
        float resBase = 32.0f * 0.8f * valScale;
        Gui::DrawGuiText(resStr,
            { vbPos.x + (valueBoxW - resW) * 0.5f,
              vbPos.y + (valueBoxH - resH) * 0.5f - resBase + resH * 0.5f },
            valScale, { 1.0f, 1.0f, 1.0f, 1.0f });
    }

    // Anti-Aliasing
    {
        float       rowY = rowStart + rowH;
        std::string lbl = "Anti-Aliasing";
        float       lblH = Gui::MeasureTextHeight(lbl, labelScale);
        float       baselineOff = 32.0f * 0.8f * labelScale;
        Gui::DrawGuiText(lbl,
            { labelX, rowY + (arrowH - lblH) * 0.5f - baselineOff + lblH * 0.5f },
            labelScale, { 0.85f, 0.85f, 0.90f, 1.0f });

        glm::vec2 leftPos = { controlsX, rowY };
        glm::vec2 rightPos = { controlsX + arrowW + valueBoxW + 8.0f * baseScale, rowY };
        glm::vec2 arrowSize = { arrowW, arrowH };

        bool hovL = mouse.x >= leftPos.x && mouse.x <= leftPos.x + arrowW &&
            mouse.y >= leftPos.y && mouse.y <= leftPos.y + arrowH;
        bool hovR = mouse.x >= rightPos.x && mouse.x <= rightPos.x + arrowW &&
            mouse.y >= rightPos.y && mouse.y <= rightPos.y + arrowH;

        m_MsaaLeftBtnScale += ((hovL ? 1.08f : 1.0f) - m_MsaaLeftBtnScale) * dt * animSpeed;
        m_MsaaRightBtnScale += ((hovR ? 1.08f : 1.0f) - m_MsaaRightBtnScale) * dt * animSpeed;

        if (DrawScaledButton("<", leftPos, arrowSize, m_MsaaLeftBtnScale, baseScale,
            { 0.25f, 0.25f, 0.30f, 1.0f }, { 0.40f, 0.40f, 0.50f, 1.0f }, hovL))
        {
            m_PendingMsaaIndex = (m_PendingMsaaIndex - 1 + MsaaOptionCount) % MsaaOptionCount;
        }
        if (DrawScaledButton(">", rightPos, arrowSize, m_MsaaRightBtnScale, baseScale,
            { 0.25f, 0.25f, 0.30f, 1.0f }, { 0.40f, 0.40f, 0.50f, 1.0f }, hovR))
        {
            m_PendingMsaaIndex = (m_PendingMsaaIndex + 1) % MsaaOptionCount;
        }

        glm::vec2 vbPos = { controlsX + arrowW + 8.0f * baseScale,
                            rowY + (arrowH - valueBoxH) * 0.5f };
        Gui::Panel(vbPos, { valueBoxW, valueBoxH }, { 0.18f, 0.18f, 0.22f, 1.0f }, 10.0f * baseScale);

        int         msaaSamples = MsaaOptions[m_PendingMsaaIndex];
        std::string msaaStr = (msaaSamples == 1) ? "OFF" : ("x" + std::to_string(msaaSamples));
        float valScale = 0.85f * baseScale;
        float mW = Gui::MeasureTextWidth(msaaStr, valScale);
        float mH = Gui::MeasureTextHeight(msaaStr, valScale);
        float mBase = 32.0f * 0.8f * valScale;
        Gui::DrawGuiText(msaaStr,
            { vbPos.x + (valueBoxW - mW) * 0.5f,
              vbPos.y + (valueBoxH - mH) * 0.5f - mBase + mH * 0.5f },
            valScale, { 1.0f, 1.0f, 1.0f, 1.0f });
    }

    // Przyciski back i apply w menu settings
    float     bottomY = panelY + panelH - 80.0f * baseScale;
    float     smallBtnW = 180.0f * baseScale;
    float     smallBtnH = 56.0f * baseScale;
    glm::vec2 backPos = { panelX + 40.0f * baseScale, bottomY };
    glm::vec2 applyPos = { panelX + panelW - smallBtnW - 40.0f * baseScale, bottomY };
    glm::vec2 sbSize = { smallBtnW, smallBtnH };

    bool hovBack = mouse.x >= backPos.x && mouse.x <= backPos.x + smallBtnW &&
        mouse.y >= backPos.y && mouse.y <= backPos.y + smallBtnH;
    bool hovApply = mouse.x >= applyPos.x && mouse.x <= applyPos.x + smallBtnW &&
        mouse.y >= applyPos.y && mouse.y <= applyPos.y + smallBtnH;

    m_BackBtnScale += ((hovBack ? 1.05f : 1.0f) - m_BackBtnScale) * dt * animSpeed;
    m_ApplyBtnScale += ((hovApply ? 1.05f : 1.0f) - m_ApplyBtnScale) * dt * animSpeed;

    if (DrawScaledButton("BACK", backPos, sbSize, m_BackBtnScale, baseScale,
        { 0.28f, 0.28f, 0.32f, 1.0f }, { 0.42f, 0.42f, 0.48f, 1.0f }, hovBack))
    {
        // Wracamy do głównego menu
        m_SettingsOpen = false;
    }

    if (DrawScaledButton("APPLY", applyPos, sbSize, m_ApplyBtnScale, baseScale,
        { 0.15f, 0.50f, 0.18f, 1.0f }, { 0.20f, 0.70f, 0.25f, 1.0f }, hovApply))
    {
        auto& gs = GraphicsSettings::Get();
        gs.MsaaSamples = MsaaOptions[m_PendingMsaaIndex];
        gs.WindowWidth = GraphicsSettings::Resolutions[m_PendingResIndex].first;
        gs.WindowHeight = GraphicsSettings::Resolutions[m_PendingResIndex].second;

        Application::Get().ApplyGraphicsSettings();

        m_ViewportWidth = (float)gs.WindowWidth;
        m_ViewportHeight = (float)gs.WindowHeight;

        m_SettingsOpen = false;
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

    if (e.GetEventType() == EventType::MouseButtonPressed) {
        e.Handled = true;
    }
}

bool MainMenuLayer::OnWindowResize(WindowResizeEvent& e) {
    m_ViewportWidth = (float)e.GetWidth();
    m_ViewportHeight = (float)e.GetHeight();
    Gui::SetScreenSize(m_ViewportWidth, m_ViewportHeight);
    return false;
}