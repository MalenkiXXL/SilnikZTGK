#include "SettingsMenuPanel.h"
#include "CookingStation/Layers/GuiLayer/Utils/Gui.h"
#include "CookingStation/Core/GraphicsSettings.h"
#include "CookingStation/Core/Application.h"
#include "CookingStation/Core/Input.h"
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
}

void SettingsMenuPanel::OnUpdate(float dt) {
    m_DeltaTime = dt;
}

void SettingsMenuPanel::Draw(float baseScale) {
    if (!m_IsVisible) return;

    auto windowSize = Input::GetWindowSize();
    float screenW = (float)windowSize.first;
    float screenH = (float)windowSize.second;

    glm::vec2 panelSize = { 800.0f * baseScale, 550.0f * baseScale };
    glm::vec2 panelPos = { (screenW - panelSize.x) * 0.5f, (screenH - panelSize.y) * 0.5f };
    Gui::Panel(panelPos, panelSize, { 0.12f, 0.12f, 0.15f, 0.95f }, 20.0f * baseScale);

    float titleScale = 1.0f * baseScale;
    float titleW = Gui::MeasureTextWidth("USTAWIENIA GRAFICZNE", titleScale);
    Gui::DrawGuiText("USTAWIENIA GRAFICZNE", { panelPos.x + (panelSize.x - titleW) * 0.5f, panelPos.y + 60.0f * baseScale }, titleScale, { 1.0f, 0.8f, 0.2f, 1.0f });

    glm::vec2 mouse = Gui::GetMappedMousePos();
    auto isHov = [&](glm::vec2 p, glm::vec2 s) {
        return mouse.x >= p.x && mouse.x <= p.x + s.x && mouse.y >= p.y && mouse.y <= p.y + s.y;
        };

    float startY = panelPos.y + 180.0f * baseScale;
    float leftColX = panelPos.x + 80.0f * baseScale;
    float rightColX = panelPos.x + 350.0f * baseScale;
    glm::vec2 arrowSize = { 50.0f * baseScale, 50.0f * baseScale };

    // --- ROZDZIELCZOŚĆ ---
    Gui::DrawGuiText("Rozdzielczosc:", { leftColX, startY }, 0.7f * baseScale, { 0.9f, 0.9f, 0.9f, 1.0f });

    glm::vec2 resLeftPos = { rightColX, startY - 35.0f * baseScale };
    bool hovResL = isHov(resLeftPos, arrowSize);
    if (GuiUtils::DrawScaledButton("<", resLeftPos, arrowSize, m_ResLeftBtnScale, hovResL ? 1.15f : 1.0f, baseScale, { 0.3f, 0.3f, 0.3f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }, hovResL, m_DeltaTime)) {
        if (m_PendingResIndex > 0) m_PendingResIndex--;
    }

    std::string resText = std::to_string(GraphicsSettings::Resolutions[m_PendingResIndex].first) + " x " + std::to_string(GraphicsSettings::Resolutions[m_PendingResIndex].second);
    Gui::DrawGuiText(resText, { rightColX + 70.0f * baseScale, startY }, 0.7f * baseScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    glm::vec2 resRightPos = { rightColX + 260.0f * baseScale, startY - 35.0f * baseScale };
    bool hovResR = isHov(resRightPos, arrowSize);
    if (GuiUtils::DrawScaledButton(">", resRightPos, arrowSize, m_ResRightBtnScale, hovResR ? 1.15f : 1.0f, baseScale, { 0.3f, 0.3f, 0.3f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }, hovResR, m_DeltaTime)) {
        if (m_PendingResIndex < GraphicsSettings::ResolutionCount - 1) m_PendingResIndex++;
    }

    // --- MSAA --- (Analogicznie jak wyżej)
    float startY2 = startY + 100.0f * baseScale;
    Gui::DrawGuiText("Antialiasing:", { leftColX, startY2 }, 0.7f * baseScale, { 0.9f, 0.9f, 0.9f, 1.0f });

    glm::vec2 msaaLeftPos = { rightColX, startY2 - 35.0f * baseScale };
    bool hovMsaaL = isHov(msaaLeftPos, arrowSize);
    if (GuiUtils::DrawScaledButton("<", msaaLeftPos, arrowSize, m_MsaaLeftBtnScale, hovMsaaL ? 1.15f : 1.0f, baseScale, { 0.3f, 0.3f, 0.3f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }, hovMsaaL, m_DeltaTime)) {
        if (m_PendingMsaaIndex > 0) m_PendingMsaaIndex--;
    }

    std::string msaaText = m_MsaaOptions[m_PendingMsaaIndex] == 1 ? "Off" : "MSAA x" + std::to_string(m_MsaaOptions[m_PendingMsaaIndex]);
    Gui::DrawGuiText(msaaText, { rightColX + 70.0f * baseScale, startY2 }, 0.7f * baseScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    glm::vec2 msaaRightPos = { rightColX + 260.0f * baseScale, startY2 - 35.0f * baseScale };
    bool hovMsaaR = isHov(msaaRightPos, arrowSize);
    if (GuiUtils::DrawScaledButton(">", msaaRightPos, arrowSize, m_MsaaRightBtnScale, hovMsaaR ? 1.15f : 1.0f, baseScale, { 0.3f, 0.3f, 0.3f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }, hovMsaaR, m_DeltaTime)) {
        if (m_PendingMsaaIndex < m_MsaaOptions.size() - 1) m_PendingMsaaIndex++;
    }

    // --- PRZYCISKI DOLNE ---
    glm::vec2 btnSize = { 200.0f * baseScale, 70.0f * baseScale };

    glm::vec2 backPos = { panelPos.x + 80.0f * baseScale, panelPos.y + panelSize.y - 110.0f * baseScale };
    bool hovBack = isHov(backPos, btnSize);
    if (GuiUtils::DrawScaledButton("BACK", backPos, btnSize, m_BackBtnScale, hovBack ? 1.05f : 1.0f, baseScale, { 0.5f, 0.2f, 0.2f, 1.0f }, { 0.7f, 0.3f, 0.3f, 1.0f }, hovBack, m_DeltaTime)) {
        SetVisible(false); // Zamyka sam siebie!
    }

    glm::vec2 applyPos = { panelPos.x + panelSize.x - 280.0f * baseScale, panelPos.y + panelSize.y - 110.0f * baseScale };
    bool hovApply = isHov(applyPos, btnSize);
    if (GuiUtils::DrawScaledButton("APPLY", applyPos, btnSize, m_ApplyBtnScale, hovApply ? 1.05f : 1.0f, baseScale, { 0.2f, 0.5f, 0.2f, 1.0f }, { 0.3f, 0.7f, 0.3f, 1.0f }, hovApply, m_DeltaTime)) {
        auto& gs = GraphicsSettings::Get();
        gs.WindowWidth = GraphicsSettings::Resolutions[m_PendingResIndex].first;
        gs.WindowHeight = GraphicsSettings::Resolutions[m_PendingResIndex].second;
        gs.MsaaSamples = m_MsaaOptions[m_PendingMsaaIndex];
        Application::Get().ApplyGraphicsSettings();
    }
}