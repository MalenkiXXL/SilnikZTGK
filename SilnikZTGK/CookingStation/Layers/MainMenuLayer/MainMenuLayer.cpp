#include "MainMenuLayer.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Layers/GuiLayer/Gui.h"
#include "CookingStation/Layers/GuiLayer/Renderer2D.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Scene/SceneSerializer.h"
#include "CookingStation/Core/Application.h"
#include "CookingStation/Layers/GuiLayer/GameGuiLayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>
#include <algorithm>

void MainMenuLayer::OnAttach() {
    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = (float)windowSize.first;
    m_ViewportHeight = (float)windowSize.second;

    Gui::SetScreenSize(m_ViewportWidth, m_ViewportHeight);

    m_Background = std::make_shared<Texture>("assets://background/menu_background.png");
}

void MainMenuLayer::OnUpdate(Timestep ts) {
    if (!m_IsActive) return;

    float dt = ts.GetSeconds();

    // --- SKALA BAZOWA (analogicznie do GameGuiLayer) ---
    float baseScale = std::max(m_ViewportHeight / 1080.0f, 0.5f);

    // --- WYMIARY PRZYCISKÓW skalowane względem ekranu ---
    float btnWidth = 420.0f * baseScale;
    float btnHeight = 90.0f * baseScale;
    float btnGap = 28.0f * baseScale;

    float blockLeft = m_ViewportWidth * 0.1f;
    float blockTop = m_ViewportHeight * 0.48f;

    glm::vec2 playPos = { blockLeft, blockTop };
    glm::vec2 settingsPos = { blockLeft, blockTop + btnHeight + btnGap };
    glm::vec2 btnSize = { btnWidth, btnHeight };

    // --- HOVER ---
    glm::vec2 mouse = Gui::GetMappedMousePos();

    auto isHovered = [&](glm::vec2 pos, glm::vec2 sz) {
        return mouse.x >= pos.x && mouse.x <= pos.x + sz.x &&
            mouse.y >= pos.y && mouse.y <= pos.y + sz.y;
        };

    bool hoverPlay = isHovered(playPos, btnSize);
    bool hoverSettings = isHovered(settingsPos, btnSize);

    // --- ANIMACJA SKALI (lerp jak w GameGuiLayer::DrawBubblyImage) ---
    float animSpeed = 12.0f;
    float targetPlay = hoverPlay ? 1.04f : 1.0f;
    float targetSettings = hoverSettings ? 1.04f : 1.0f;
    m_PlayBtnScale += (targetPlay - m_PlayBtnScale) * dt * animSpeed;
    m_SettingsBtnScale += (targetSettings - m_SettingsBtnScale) * dt * animSpeed;

    // --- RENDEROWANIE ---
    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    Renderer2D::BeginScene(uiProj);

    // 1. Tło
    if (m_Background) {
        Renderer2D::DrawQuad({ 0.0f, 0.0f }, { m_ViewportWidth, m_ViewportHeight },
            m_Background, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    }
    else {
        Gui::Panel({ 0, 0 }, { m_ViewportWidth, m_ViewportHeight },
            { 0.08f, 0.08f, 0.12f, 1.0f }, 0.0f);
    }

    // -------------------------------------------------------
    // Pomocnicza lambda: rysuje przycisk ze skalą i napisem wyśrodkowanym w środku
    // -------------------------------------------------------
    auto DrawScaledButton = [&](const std::string& label,
        glm::vec2 basePos, glm::vec2 baseSize,
        float btnScale,
        glm::vec4 colorNormal, glm::vec4 colorHover,
        bool hovered) -> bool
        {
            // Oblicz skalowane wymiary i wyrównaj środek
            glm::vec2 scaledSize = baseSize * btnScale;
            glm::vec2 scaledPos = {
                basePos.x + (baseSize.x - scaledSize.x) * 0.5f,
                basePos.y + (baseSize.y - scaledSize.y) * 0.5f
            };

            glm::vec4 bgColor = hovered ? colorHover : colorNormal;
            if (hovered && Input::IsMouseButtonPressed(0))
                bgColor = bgColor * glm::vec4(0.75f, 0.75f, 0.75f, 1.0f);

            // Cień przycisku
            Gui::Panel(scaledPos + glm::vec2(4.0f * baseScale, 5.0f * baseScale),
                scaledSize,
                { 0.0f, 0.0f, 0.0f, 0.35f }, 15.0f * baseScale);

            // Tło przycisku
            Gui::Panel(scaledPos, scaledSize, bgColor, 15.0f * baseScale);

            // Tekst wyśrodkowany wewnątrz przycisku
            float textScale = 1.3f * baseScale;
            float textWidth = Gui::MeasureTextWidth(label, textScale);
            float textHeight = Gui::MeasureTextHeight(label, textScale);

            // DrawGuiText rysuje od górnego-lewego + wewnętrzny offset bazowy (baselineOffset)
            // Aby wyśrodkować: środek przycisku minus pół szerokości tekstu
            float baselineOffset = 32.0f * 0.8f * textScale;  // jak w Gui::DrawGuiText
            glm::vec2 textPos = {
                scaledPos.x + (scaledSize.x - textWidth) * 0.5f,
                scaledPos.y + (scaledSize.y - textHeight) * 0.5f - baselineOffset + (textHeight * 0.5f)
            };

            // Cień napisu
            Gui::DrawGuiText(label, textPos + glm::vec2(2.0f, 2.0f), textScale, { 0.0f, 0.0f, 0.0f, 0.7f });
            // Napis
            Gui::DrawGuiText(label, textPos, textScale, { 1.0f, 1.0f, 1.0f, 1.0f });

            return hovered && Input::IsMouseButtonJustPressed(0);
        };

    // 2. Przycisk PLAY
    bool clickedPlay = DrawScaledButton(
        "PLAY",
        playPos, btnSize, m_PlayBtnScale,
        { 0.18f, 0.62f, 0.22f, 1.0f },
        { 0.25f, 0.80f, 0.30f, 1.0f },
        hoverPlay
    );
    if (clickedPlay) PlayGame();

    // 3. Przycisk SETTINGS
    DrawScaledButton(
        "SETTINGS",
        settingsPos, btnSize, m_SettingsBtnScale,
        { 0.28f, 0.28f, 0.32f, 1.0f },
        { 0.42f, 0.42f, 0.48f, 1.0f },
        hoverSettings
    );

    Renderer2D::EndScene();
    glEnable(GL_DEPTH_TEST);
}

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

void MainMenuLayer::OnEvent(Event& e) {
    // WindowResize zawsze obsługujemy, nawet gdy menu nieaktywne
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
        return OnWindowResize(ev);
        });

    // Blokujemy eventy myszy TYLKO gdy menu jest aktywne
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