#include "PauseMenuPanel.h"
#include "CookingStation/Layers/GuiLayer/Utils/Gui.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Core/Application.h"
#include "CookingStation/Events/KeyEvent.h"
#include "../Utils/GuiUtils.h"

PauseMenuPanel::PauseMenuPanel() {
    m_SettingsPanel = std::make_unique<SettingsMenuPanel>();
}

void PauseMenuPanel::TogglePause() {
    if (m_SettingsPanel->IsVisible()) {
        m_SettingsPanel->SetVisible(false); // Zamknij tylko ustawienia
    }
    else {
        m_IsPaused = !m_IsPaused;
        auto activeScene = SceneManager::GetActiveScene();
        if (activeScene) {
            activeScene->SetState(m_IsPaused ? SceneState::Pause : SceneState::Play);
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

    // Zablokuj kliknięcia pod menu
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

    // Wpierw poszarzenie tła ekranu
    auto windowSize = Input::GetWindowSize();
    Gui::Panel({ 0.0f, 0.0f }, { (float)windowSize.first, (float)windowSize.second }, { 0.05f, 0.05f, 0.05f, 0.75f }, 0.0f);

    // Jeśli ustawienia są widoczne, rysuj TYLKO je i wyjdź
    if (m_SettingsPanel->IsVisible()) {
        m_SettingsPanel->Draw(baseScale);
        return;
    }

    // W przeciwnym razie rysuj główne guziki
    float btnWidth = 350.0f * baseScale;
    float btnHeight = 80.0f * baseScale;
    float btnGap = 25.0f * baseScale;

    float btnX = (windowSize.first - btnWidth) * 0.5f;
    float totalH = (3.0f * btnHeight) + (2.0f * btnGap);
    float startY = (windowSize.second - totalH) * 0.5f;

    glm::vec2 mouse = Gui::GetMappedMousePos();
    auto isHov = [&](glm::vec2 p, glm::vec2 s) {
        return mouse.x >= p.x && mouse.x <= p.x + s.x && mouse.y >= p.y && mouse.y <= p.y + s.y;
        };

    glm::vec2 btnSize = { btnWidth, btnHeight };

    // RETURN
    glm::vec2 retPos = { btnX, startY };
    bool hoverRet = isHov(retPos, btnSize);
    if (GuiUtils::DrawScaledButton("RETURN", retPos, btnSize, m_ReturnBtnScale, hoverRet ? 1.05f : 1.0f, baseScale, { 0.18f, 0.62f, 0.22f, 1.0f }, { 0.25f, 0.80f, 0.30f, 1.0f }, hoverRet, m_DeltaTime)) {
        TogglePause();
    }

    // SETTINGS
    glm::vec2 setPos = { btnX, startY + btnHeight + btnGap };
    bool hoverSet = isHov(setPos, btnSize);
    if (GuiUtils::DrawScaledButton("SETTINGS", setPos, btnSize, m_SettingsBtnScale, hoverSet ? 1.05f : 1.0f, baseScale, { 0.28f, 0.28f, 0.32f, 1.0f }, { 0.42f, 0.42f, 0.48f, 1.0f }, hoverSet, m_DeltaTime)) {
        m_SettingsPanel->SyncWithEngine();
        m_SettingsPanel->SetVisible(true);
    }

    // EXIT
    glm::vec2 exitPos = { btnX, startY + 2.0f * (btnHeight + btnGap) };
    bool hoverExit = isHov(exitPos, btnSize);
    if (GuiUtils::DrawScaledButton("EXIT", exitPos, btnSize, m_ExitBtnScale, hoverExit ? 1.05f : 1.0f, baseScale, { 0.70f, 0.20f, 0.20f, 1.0f }, { 0.85f, 0.30f, 0.30f, 1.0f }, hoverExit, m_DeltaTime)) {
        m_IsPaused = false;
        SceneManager::NewScene();
        Application::Get().GetEventBus().Publish(ShowMainMenuEvent{});
    }
}