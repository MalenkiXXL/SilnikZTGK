#include "PauseMenuPanel.h"
#include "CookingStation/Layers/GuiLayer/Utils/Gui.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Core/Application.h"
#include "CookingStation/Events/KeyEvent.h"
#include "../Utils/GuiUtils.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Layers/GuiLayer/Utils/Renderer2D.h"

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

    auto windowSize = Input::GetWindowSize();

    // 1. Poszarzenie tła ekranu (lekki tint)
    Gui::Panel({ 0.0f, 0.0f }, { (float)windowSize.first, (float)windowSize.second }, { 0.05f, 0.05f, 0.05f, 0.75f }, 0.0f);

    // Jeśli ustawienia są widoczne, rysuj TYLKO je i wyjdź
    if (m_SettingsPanel->IsVisible()) {
        m_SettingsPanel->Draw(baseScale);
        return;
    }

    // 2. Pobranie tekstur przez AssetManager
    auto boardTex = AssetManager::GetTexture("assets://UI/cuttingBoardPrototypeVert.png");
    auto playTex = AssetManager::GetTexture("assets://UI/playButton.png");
    auto settingsTex = AssetManager::GetTexture("assets://UI/creditsButton.png");
    auto exitTex = AssetManager::GetTexture("assets://UI/exitButton.png");

    glm::vec2 uv0 = { 0.0f, 1.0f };
    glm::vec2 uv1 = { 1.0f, 0.0f };

    // NAPRAWA: Zamiast sztywnych wielkości, obliczamy idealne proporcje z tekstury (tak jak w MainMenu!)
    auto getAspectSize = [&](const std::shared_ptr<Texture>& tex, float targetHeight) -> glm::vec2 {
        if (tex && tex->GetRendererID() != 0) {
            float aspect = (float)tex->GetWidth() / (float)tex->GetHeight();
            return { targetHeight * aspect, targetHeight };
        }
        return { targetHeight * 3.0f, targetHeight }; // Zabezpieczenie, gdyby brakło pliku
        };

    // 3. Rysowanie tła menu (Deska do krojenia) - dynamiczna szerokość chroniąca jakość
    float boardHeight = 450.0f * baseScale;
    glm::vec2 boardSize = getAspectSize(boardTex, boardHeight);

    float boardX = (windowSize.first - boardSize.x) * 0.5f;
    float boardY = (windowSize.second - boardSize.y) * 0.5f;

    if (boardTex) {
        Renderer2D::DrawQuad({ boardX, boardY }, boardSize, boardTex->GetRendererID(), { 1.0f, 1.0f, 1.0f, 0.90f }, uv0, uv1);
    }
    else {
        Gui::Panel({ boardX, boardY }, boardSize, { 0.7f, 0.5f, 0.3f, 0.85f }, 20.0f);
    }

    // 4. Parametry i pozycjonowanie przycisków 
    float btnHeight = 85.0f * baseScale;
    float btnGap = 20.0f * baseScale;

    // Każdy przycisk wylicza swoją własną, idealną szerokość!
    glm::vec2 playSize = getAspectSize(playTex, btnHeight);
    glm::vec2 settingsSize = getAspectSize(settingsTex, btnHeight);
    glm::vec2 exitSize = getAspectSize(exitTex, btnHeight);

    float totalH = (3.0f * btnHeight) + (2.0f * btnGap);
    float startY = (windowSize.second - totalH) * 0.5f;

    glm::vec2 mouse = Gui::GetMappedMousePos();
    auto isHov = [&](glm::vec2 p, glm::vec2 s) {
        return mouse.x >= p.x && mouse.x <= p.x + s.x && mouse.y >= p.y && mouse.y <= p.y + s.y;
        };

    static bool s_LastMouseState = false;
    bool currentMouseState = Input::IsMouseButtonPressed(0);
    bool mouseClicked = currentMouseState && !s_LastMouseState;

    // Funkcja lambdy rysująca grafikę bez niszczenia jej jakości
    auto drawImageBtn = [&](auto tex, glm::vec2 basePos, glm::vec2 baseSize, float& scaleVar, bool hovered) {
        float targetScale = hovered ? 1.05f : 1.0f;
        scaleVar += (targetScale - scaleVar) * 15.0f * m_DeltaTime;

        glm::vec2 scaledSize = baseSize * scaleVar;
        glm::vec2 offset = (baseSize - scaledSize) * 0.5f;
        glm::vec2 finalPos = basePos + offset;

        if (tex) {
            Renderer2D::DrawQuad(finalPos, scaledSize, tex->GetRendererID(), { 1.0f, 1.0f, 1.0f, 1.0f }, uv0, uv1);
        }
        else {
            Gui::Panel(finalPos, scaledSize, { 1.0f, 0.0f, 1.0f, 1.0f }, 10.0f);
        }

        return hovered && mouseClicked;
        };

    // ===================================
    // RYSOWANIE PRZYCISKÓW I OBSŁUGA LOGIKI
    // ===================================

    // RETURN (Centrujemy opierając się na dynamicznie obliczonej szerokości playSize.x)
    float retX = (windowSize.first - playSize.x) * 0.5f;
    glm::vec2 retPos = { retX, startY };
    bool hoverRet = isHov(retPos, playSize);
    if (drawImageBtn(playTex, retPos, playSize, m_ReturnBtnScale, hoverRet)) {
        TogglePause();
    }

    // SETTINGS (Centrujemy opierając się na dynamicznie obliczonej szerokości settingsSize.x)
    float setX = (windowSize.first - settingsSize.x) * 0.5f;
    glm::vec2 setPos = { setX, startY + btnHeight + btnGap };
    bool hoverSet = isHov(setPos, settingsSize);
    if (drawImageBtn(settingsTex, setPos, settingsSize, m_SettingsBtnScale, hoverSet)) {
        m_SettingsPanel->SyncWithEngine();
        m_SettingsPanel->SetVisible(true);
    }

    // EXIT (Centrujemy opierając się na dynamicznie obliczonej szerokości exitSize.x)
    float exitX = (windowSize.first - exitSize.x) * 0.5f;
    glm::vec2 exitPos = { exitX, startY + 2.0f * (btnHeight + btnGap) };
    bool hoverExit = isHov(exitPos, exitSize);
    if (drawImageBtn(exitTex, exitPos, exitSize, m_ExitBtnScale, hoverExit)) {
        m_IsPaused = false;
        SceneManager::NewScene();
        Application::Get().GetEventBus().Publish(ShowMainMenuEvent{});
    }

    // Zapis stanu myszki na koniec klatki
    s_LastMouseState = currentMouseState;
}