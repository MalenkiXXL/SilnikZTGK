#include "MainMenuLayer.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Layers/GuiLayer/Gui.h"
#include "CookingStation/Layers/GuiLayer/Renderer2D.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Scene/SceneSerializer.h"
#include "CookingStation/Core/Application.h"
#include "CookingStation/Layers/GuiLayer/GameGuiLayer.h"
#include "CookingStation/Core/GraphicsSettings.h"
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <string>

constexpr int MainMenuLayer::MsaaOptions[];

// -----------------------------------------------------------------------
// OnAttach
// -----------------------------------------------------------------------
void MainMenuLayer::OnAttach() {
    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = (float)windowSize.first;
    m_ViewportHeight = (float)windowSize.second;
    Gui::SetScreenSize(m_ViewportWidth, m_ViewportHeight);

    m_Background = std::make_shared<Texture>("assets://background/menu_background.png");

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
}

// -----------------------------------------------------------------------
// Pomocnicza: przycisk ze skalą animowaną i tekstem wyśrodkowanym
// -----------------------------------------------------------------------
bool MainMenuLayer::DrawScaledButton(const std::string& label,
    glm::vec2 basePos, glm::vec2 baseSize,
    float btnScale, float bsc,
    glm::vec4 colorNormal, glm::vec4 colorHover,
    bool hovered)
{
    glm::vec2 scaledSize = baseSize * btnScale;
    glm::vec2 scaledPos = {
        basePos.x + (baseSize.x - scaledSize.x) * 0.5f,
        basePos.y + (baseSize.y - scaledSize.y) * 0.5f
    };

    glm::vec4 bgColor = hovered ? colorHover : colorNormal;
    if (hovered && Input::IsMouseButtonPressed(0))
        bgColor = bgColor * glm::vec4(0.75f, 0.75f, 0.75f, 1.0f);

    // Cien
    Gui::Panel(scaledPos + glm::vec2(4.0f * bsc, 5.0f * bsc),
        scaledSize, { 0.0f, 0.0f, 0.0f, 0.35f }, 15.0f * bsc);
    // Tlo
    Gui::Panel(scaledPos, scaledSize, bgColor, 15.0f * bsc);

    // Tekst wyśrodkowany
    float textScale = 1.3f * bsc;
    float textWidth = Gui::MeasureTextWidth(label, textScale);
    float textHeight = Gui::MeasureTextHeight(label, textScale);
    float baselineOffset = 32.0f * 0.8f * textScale;
    glm::vec2 textPos = {
        scaledPos.x + (scaledSize.x - textWidth) * 0.5f,
        scaledPos.y + (scaledSize.y - textHeight) * 0.5f - baselineOffset + (textHeight * 0.5f)
    };

    Gui::DrawGuiText(label, textPos + glm::vec2(2.0f, 2.0f), textScale, { 0.0f, 0.0f, 0.0f, 0.7f });
    Gui::DrawGuiText(label, textPos, textScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    return hovered && Input::IsMouseButtonJustPressed(0);
}

// -----------------------------------------------------------------------
// OnUpdate
// -----------------------------------------------------------------------
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

// -----------------------------------------------------------------------
// DrawMainMenu
// -----------------------------------------------------------------------
void MainMenuLayer::DrawMainMenu(float baseScale, float dt) {
    float btnWidth = 420.0f * baseScale;
    float btnHeight = 90.0f * baseScale;
    float btnGap = 28.0f * baseScale;
    float blockLeft = m_ViewportWidth * 0.1f;
    float blockTop = m_ViewportHeight * 0.48f;

    glm::vec2 playPos = { blockLeft, blockTop };
    glm::vec2 settingsPos = { blockLeft, blockTop + btnHeight + btnGap };
    glm::vec2 btnSize = { btnWidth, btnHeight };
    glm::vec2 mouse = Gui::GetMappedMousePos();

    auto isHov = [&](glm::vec2 p, glm::vec2 s) {
        return mouse.x >= p.x && mouse.x <= p.x + s.x &&
            mouse.y >= p.y && mouse.y <= p.y + s.y;
        };
    bool hoverPlay = isHov(playPos, btnSize);
    bool hoverSettings = isHov(settingsPos, btnSize);

    float animSpeed = 12.0f;
    m_PlayBtnScale += ((hoverPlay ? 1.04f : 1.0f) - m_PlayBtnScale) * dt * animSpeed;
    m_SettingsBtnScale += ((hoverSettings ? 1.04f : 1.0f) - m_SettingsBtnScale) * dt * animSpeed;

    if (DrawScaledButton("PLAY", playPos, btnSize, m_PlayBtnScale, baseScale,
        { 0.18f, 0.62f, 0.22f, 1.0f }, { 0.25f, 0.80f, 0.30f, 1.0f }, hoverPlay))
    {
        PlayGame();
    }

    if (DrawScaledButton("SETTINGS", settingsPos, btnSize, m_SettingsBtnScale, baseScale,
        { 0.28f, 0.28f, 0.32f, 1.0f }, { 0.42f, 0.42f, 0.48f, 1.0f }, hoverSettings))
    {
        m_SettingsOpen = true;
    }
}

// -----------------------------------------------------------------------
// DrawSettingsPanel
// -----------------------------------------------------------------------
void MainMenuLayer::DrawSettingsPanel(float baseScale, float dt) {
    glm::vec2 mouse = Gui::GetMappedMousePos();

    // --- Panel tla ---
    float panelW = 700.0f * baseScale;
    float panelH = 500.0f * baseScale;
    float panelX = (m_ViewportWidth - panelW) * 0.5f;
    float panelY = (m_ViewportHeight - panelH) * 0.5f;

    // Ciemne tlo z cienka ramka
    Gui::Panel({ panelX - 3.0f, panelY - 3.0f }, { panelW + 6.0f, panelH + 6.0f },
        { 0.6f, 0.6f, 0.7f, 0.5f }, 20.0f * baseScale);
    Gui::Panel({ panelX, panelY }, { panelW, panelH },
        { 0.10f, 0.10f, 0.14f, 0.97f }, 18.0f * baseScale);

    // --- Naglowek ---
    float titleScale = 1.6f * baseScale;
    std::string title = "SETTINGS";
    float titleW = Gui::MeasureTextWidth(title, titleScale);
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
    float controlsX = panelX + panelW * 0.45f;   // lewa krawedz kontrolek

    float labelScale = 0.9f * baseScale;
    float animSpeed = 14.0f;

    // ====================================================
    // RZAD 1: Rozdzielczosc
    // ====================================================
    {
        float rowY = rowStart;

        // Etykieta
        std::string lbl = "Resolution";
        float lblH = Gui::MeasureTextHeight(lbl, labelScale);
        float baselineOff = 32.0f * 0.8f * labelScale;
        Gui::DrawGuiText(lbl,
            { labelX, rowY + (arrowH - lblH) * 0.5f - baselineOff + lblH * 0.5f },
            labelScale, { 0.85f, 0.85f, 0.90f, 1.0f });

        // Strzalka lewo
        glm::vec2 leftPos = { controlsX, rowY + (arrowH - arrowH) * 0.5f };
        glm::vec2 rightPos = { controlsX + arrowW + valueBoxW + 8.0f * baseScale, rowY };
        glm::vec2 arrowSize = { arrowW, arrowH };

        bool hovL = mouse.x >= leftPos.x && mouse.x <= leftPos.x + arrowW &&
            mouse.y >= leftPos.y && mouse.y <= leftPos.y + arrowH;
        bool hovR = mouse.x >= rightPos.x && mouse.x <= rightPos.x + arrowW &&
            mouse.y >= rightPos.y && mouse.y <= rightPos.y + arrowH;

        m_ResLeftBtnScale += ((hovL ? 1.08f : 1.0f) - m_ResLeftBtnScale) * dt * animSpeed;
        m_ResRightBtnScale += ((hovR ? 1.08f : 1.0f) - m_ResRightBtnScale) * dt * animSpeed;

        // Strzalki jako przyciski
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

        // Pole wartosci
        glm::vec2 vbPos = { controlsX + arrowW + 8.0f * baseScale, rowY + (arrowH - valueBoxH) * 0.5f };
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

    // ====================================================
    // RZAD 2: Anti-Aliasing (MSAA)
    // ====================================================
    {
        float rowY = rowStart + rowH;

        std::string lbl = "Anti-Aliasing";
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

        glm::vec2 vbPos = { controlsX + arrowW + 8.0f * baseScale, rowY + (arrowH - valueBoxH) * 0.5f };
        Gui::Panel(vbPos, { valueBoxW, valueBoxH }, { 0.18f, 0.18f, 0.22f, 1.0f }, 10.0f * baseScale);

        int msaaSamples = MsaaOptions[m_PendingMsaaIndex];
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

    // ====================================================
    // Przyciski APPLY i BACK
    // ====================================================
    float bottomY = panelY + panelH - 80.0f * baseScale;
    float smallBtnW = 180.0f * baseScale;
    float smallBtnH = 56.0f * baseScale;

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
        m_SettingsOpen = false;
    }

    if (DrawScaledButton("APPLY", applyPos, sbSize, m_ApplyBtnScale, baseScale,
        { 0.15f, 0.50f, 0.18f, 1.0f }, { 0.20f, 0.70f, 0.25f, 1.0f }, hovApply))
    {
        // Zapisz do singletona i popros Application o zastosowanie
        auto& gs = GraphicsSettings::Get();
        gs.MsaaSamples = MsaaOptions[m_PendingMsaaIndex];
        gs.WindowWidth = GraphicsSettings::Resolutions[m_PendingResIndex].first;
        gs.WindowHeight = GraphicsSettings::Resolutions[m_PendingResIndex].second;

        Application::Get().ApplyGraphicsSettings();

        // Zaktualizuj lokalne wymiary (po resize okno wyśle event, ale na wszelki wypadek)
        m_ViewportWidth = (float)gs.WindowWidth;
        m_ViewportHeight = (float)gs.WindowHeight;

        m_SettingsOpen = false;
    }
}

// -----------------------------------------------------------------------
// PlayGame
// -----------------------------------------------------------------------
void MainMenuLayer::PlayGame() {
    m_IsActive = false;

    auto activeScene = SceneManager::NewScene();
    SceneSerializer serializer(activeScene.get());

    if (serializer.Deserialize("assets://levels/level02.json")) {
        activeScene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
        Gui::SetScreenSize(m_ViewportWidth, m_ViewportHeight);
        activeScene->SetState(SceneState::Play);
        activeScene->OnRuntimeStart();

        for (auto* layer : Application::Get().m_LayerStack) {
            if (layer->GetName() == "GameGuiLayer") {
                ((GameGuiLayer*)layer)->SetVisible(true);
                break;
            }
        }
        spdlog::info("Pomyslnie zaladowano level02.json!");
    }
    else {
        spdlog::error("Blad: Nie udalo sie wczytac level02.json");
    }
}

// -----------------------------------------------------------------------
// OnEvent
// -----------------------------------------------------------------------
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