#include "SettingsMenuPanel.h"
#include "CookingStation/Layers/GuiLayer/Utils/Gui.h"
#include "CookingStation/Core/GraphicsSettings.h"
#include "CookingStation/Core/Application.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Events/GameEvents.h"
#include "../Utils/GuiUtils.h"

SettingsMenuPanel::SettingsMenuPanel() {
    SyncWithEngine();
}

void SettingsMenuPanel::SyncWithEngine() {
    auto& gs = GraphicsSettings::Get();
    for (int i = 0; i < GraphicsSettings::ResolutionCount; i++) {
        if (GraphicsSettings::Resolutions[i].first == gs.WindowWidth &&
            GraphicsSettings::Resolutions[i].second == gs.WindowHeight) {
            m_PendingResIndex = i;
            break;
        }
    }
    for (size_t i = 0; i < m_MsaaOptions.size(); i++) {
        if (m_MsaaOptions[i] == gs.MsaaSamples) {
            m_PendingMsaaIndex = (int)i;
            break;
        }
    }

    m_PendingMusicEnabled = AudioEngine::IsMusicEnabled();
    m_PendingSoundsEnabled = AudioEngine::AreSoundsEnabled();
}

void SettingsMenuPanel::OnUpdate(float dt) {
    m_DeltaTime = dt;
}

void SettingsMenuPanel::Draw(float baseScale) {
    if (!m_IsVisible) return;

    auto windowSize = Input::GetWindowSize();
    float screenW = (float)windowSize.first;
    float screenH = (float)windowSize.second;

    glm::vec2 panelSize = { 800.0f * baseScale, 750.0f * baseScale };
    glm::vec2 panelPos = { (screenW - panelSize.x) * 0.5f, (screenH - panelSize.y) * 0.5f };
    Gui::Panel(panelPos, panelSize, { 0.12f, 0.12f, 0.15f, 0.95f }, 20.0f * baseScale);

    float titleScale = 1.0f * baseScale;
    float titleW = Gui::MeasureTextWidth("USTAWIENIA", titleScale);
    Gui::DrawGuiText("USTAWIENIA", { panelPos.x + (panelSize.x - titleW) * 0.5f, panelPos.y + 60.0f * baseScale }, titleScale, { 1.0f, 0.8f, 0.2f, 1.0f });

    glm::vec2 mouse = Gui::GetMappedMousePos();
    auto isHov = [&](glm::vec2 p, glm::vec2 s) {
        return mouse.x >= p.x && mouse.x <= p.x + s.x && mouse.y >= p.y && mouse.y <= p.y + s.y;
        };

    float startY = panelPos.y + 180.0f * baseScale;
    float leftColX = panelPos.x + 80.0f * baseScale;
    float rightColX = panelPos.x + 350.0f * baseScale;
    glm::vec2 arrowSize = { 50.0f * baseScale, 50.0f * baseScale };

    // --- RZĄD 1: ROZDZIELCZOŚĆ ---
    Gui::DrawGuiText("Rozdzielczosc:", { leftColX, startY }, 0.7f * baseScale, { 0.9f, 0.9f, 0.9f, 1.0f });

    glm::vec2 resLeftPos = { rightColX, startY - 35.0f * baseScale };
    bool hovResL = isHov(resLeftPos, arrowSize);
    if (GuiUtils::DrawScaledButton("<", resLeftPos, arrowSize, m_ResLeftBtnScale, hovResL ? 1.15f : 1.0f, baseScale, { 0.3f, 0.3f, 0.3f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }, hovResL, m_DeltaTime)) {

        m_PendingResIndex = (m_PendingResIndex - 1 + GraphicsSettings::ResolutionCount) % GraphicsSettings::ResolutionCount;
    }

    std::string resText = std::to_string(GraphicsSettings::Resolutions[m_PendingResIndex].first) + " x " + std::to_string(GraphicsSettings::Resolutions[m_PendingResIndex].second);
    Gui::DrawGuiText(resText, { rightColX + 70.0f * baseScale, startY }, 0.7f * baseScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    glm::vec2 resRightPos = { rightColX + 260.0f * baseScale, startY - 35.0f * baseScale };
    bool hovResR = isHov(resRightPos, arrowSize);
    if (GuiUtils::DrawScaledButton(">", resRightPos, arrowSize, m_ResRightBtnScale, hovResR ? 1.15f : 1.0f, baseScale, { 0.3f, 0.3f, 0.3f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }, hovResR, m_DeltaTime)) {
        m_PendingResIndex = (m_PendingResIndex + 1) % GraphicsSettings::ResolutionCount;
    }

    // --- RZĄD 2: ANTI-ALIASING ---
    float startY2 = startY + 100.0f * baseScale;
    Gui::DrawGuiText("Antialiasing:", { leftColX, startY2 }, 0.7f * baseScale, { 0.9f, 0.9f, 0.9f, 1.0f });

    glm::vec2 msaaLeftPos = { rightColX, startY2 - 35.0f * baseScale };
    bool hovMsaaL = isHov(msaaLeftPos, arrowSize);
    if (GuiUtils::DrawScaledButton("<", msaaLeftPos, arrowSize, m_MsaaLeftBtnScale, hovMsaaL ? 1.15f : 1.0f, baseScale, { 0.3f, 0.3f, 0.3f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }, hovMsaaL, m_DeltaTime)) {
        m_PendingMsaaIndex = (m_PendingMsaaIndex - 1 + m_MsaaOptions.size()) % m_MsaaOptions.size();
    }

    std::string msaaText = m_MsaaOptions[m_PendingMsaaIndex] == 1 ? "Off" : "MSAA x" + std::to_string(m_MsaaOptions[m_PendingMsaaIndex]);
    Gui::DrawGuiText(msaaText, { rightColX + 70.0f * baseScale, startY2 }, 0.7f * baseScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    glm::vec2 msaaRightPos = { rightColX + 260.0f * baseScale, startY2 - 35.0f * baseScale };
    bool hovMsaaR = isHov(msaaRightPos, arrowSize);
    if (GuiUtils::DrawScaledButton(">", msaaRightPos, arrowSize, m_MsaaRightBtnScale, hovMsaaR ? 1.15f : 1.0f, baseScale, { 0.3f, 0.3f, 0.3f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }, hovMsaaR, m_DeltaTime)) {
        m_PendingMsaaIndex = (m_PendingMsaaIndex + 1) % m_MsaaOptions.size();
    }

    // --- RZĄD 3: MUZYKA (Tło) ---
    float startY3 = startY2 + 100.0f * baseScale;
    Gui::DrawGuiText("Muzyka:", { leftColX, startY3 }, 0.7f * baseScale, { 0.9f, 0.9f, 0.9f, 1.0f });

    glm::vec2 musLeftPos = { rightColX, startY3 - 35.0f * baseScale };
    bool hovMusL = isHov(musLeftPos, arrowSize);
    if (GuiUtils::DrawScaledButton("<", musLeftPos, arrowSize, m_MusicLeftBtnScale, hovMusL ? 1.15f : 1.0f, baseScale, { 0.3f, 0.3f, 0.3f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }, hovMusL, m_DeltaTime)) {
        m_PendingMusicEnabled = !m_PendingMusicEnabled;
    }

    std::string musText = m_PendingMusicEnabled ? "ON" : "OFF";
    Gui::DrawGuiText(musText, { rightColX + 115.0f * baseScale, startY3 }, 0.7f * baseScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    glm::vec2 musRightPos = { rightColX + 260.0f * baseScale, startY3 - 35.0f * baseScale };
    bool hovMusR = isHov(musRightPos, arrowSize);
    if (GuiUtils::DrawScaledButton(">", musRightPos, arrowSize, m_MusicRightBtnScale, hovMusR ? 1.15f : 1.0f, baseScale, { 0.3f, 0.3f, 0.3f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }, hovMusR, m_DeltaTime)) {
        m_PendingMusicEnabled = !m_PendingMusicEnabled;
    }

    // --- RZĄD 4: DŹWIĘKI (SFX) ---
    float startY4 = startY3 + 100.0f * baseScale;
    Gui::DrawGuiText("Dzwieki:", { leftColX, startY4 }, 0.7f * baseScale, { 0.9f, 0.9f, 0.9f, 1.0f });

    glm::vec2 sndLeftPos = { rightColX, startY4 - 35.0f * baseScale };
    bool hovSndL = isHov(sndLeftPos, arrowSize);
    if (GuiUtils::DrawScaledButton("<", sndLeftPos, arrowSize, m_SoundsLeftBtnScale, hovSndL ? 1.15f : 1.0f, baseScale, { 0.3f, 0.3f, 0.3f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }, hovSndL, m_DeltaTime)) {
        m_PendingSoundsEnabled = !m_PendingSoundsEnabled;
    }

    std::string sndText = m_PendingSoundsEnabled ? "ON" : "OFF";
    Gui::DrawGuiText(sndText, { rightColX + 115.0f * baseScale, startY4 }, 0.7f * baseScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    glm::vec2 sndRightPos = { rightColX + 260.0f * baseScale, startY4 - 35.0f * baseScale };
    bool hovSndR = isHov(sndRightPos, arrowSize);
    if (GuiUtils::DrawScaledButton(">", sndRightPos, arrowSize, m_SoundsRightBtnScale, hovSndR ? 1.15f : 1.0f, baseScale, { 0.3f, 0.3f, 0.3f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }, hovSndR, m_DeltaTime)) {
        m_PendingSoundsEnabled = !m_PendingSoundsEnabled;
    }

    // --- DOLNY PANEL ---
    glm::vec2 btnSize = { 200.0f * baseScale, 70.0f * baseScale };

    glm::vec2 backPos = { panelPos.x + 80.0f * baseScale, panelPos.y + panelSize.y - 110.0f * baseScale };
    bool hovBack = isHov(backPos, btnSize);
    if (GuiUtils::DrawScaledButton("BACK", backPos, btnSize, m_BackBtnScale, hovBack ? 1.05f : 1.0f, baseScale, { 0.5f, 0.2f, 0.2f, 1.0f }, { 0.7f, 0.3f, 0.3f, 1.0f }, hovBack, m_DeltaTime)) {
        SetVisible(false);
    }

    glm::vec2 applyPos = { panelPos.x + panelSize.x - 280.0f * baseScale, panelPos.y + panelSize.y - 110.0f * baseScale };
    bool hovApply = isHov(applyPos, btnSize);
    if (GuiUtils::DrawScaledButton("APPLY", applyPos, btnSize, m_ApplyBtnScale, hovApply ? 1.05f : 1.0f, baseScale, { 0.2f, 0.5f, 0.2f, 1.0f }, { 0.3f, 0.7f, 0.3f, 1.0f }, hovApply, m_DeltaTime)) {

        Application::Get().GetEventBus().Publish(AudioSettingsChangedEvent{
            m_PendingMusicEnabled,
            m_PendingSoundsEnabled
            });

        auto& gs = GraphicsSettings::Get();
        int newWidth = GraphicsSettings::Resolutions[m_PendingResIndex].first;
        int newHeight = GraphicsSettings::Resolutions[m_PendingResIndex].second;
        int newMsaa = m_MsaaOptions[m_PendingMsaaIndex];

        if (gs.WindowWidth != newWidth || gs.WindowHeight != newHeight || gs.MsaaSamples != newMsaa) {

            gs.WindowWidth = newWidth;
            gs.WindowHeight = newHeight;
            gs.MsaaSamples = newMsaa;

            Application::Get().ApplyGraphicsSettings();
        }
    }
}