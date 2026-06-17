#include "SettingsMenuPanel.h"
#include "CookingStation/Layers/GuiLayer/Utils/Gui.h"
#include "CookingStation/Core/GraphicsSettings.h"
#include "CookingStation/Core/Application.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Events/GameEvents.h"
#include "../Utils/GuiUtils.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Layers/GuiLayer/Utils/Renderer2D.h"

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

    // --- ŁADOWANIE TEKSTUR ---
    auto boardTex = AssetManager::GetTexture("assets://UI/cuttingBoard.png");
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
    glm::vec2 boardSize = getAspectSize(boardTex, boardHeight);

    // Zabezpieczenie minimalnej szerokości, żeby elementy się zmieściły
    if (boardSize.x < 850.0f * baseScale) boardSize.x = 850.0f * baseScale;

    glm::vec2 panelPos = { (screenW - boardSize.x) * 0.5f, (screenH - boardSize.y) * 0.5f };

    if (boardTex && boardTex->GetRendererID() != 0) {
        Renderer2D::DrawQuad(panelPos, boardSize, boardTex->GetRendererID(), { 1.0f, 1.0f, 1.0f, 1.0f }, uv0, uv1);
    }
    else {
        Gui::Panel(panelPos, boardSize, { 0.12f, 0.12f, 0.15f, 0.95f }, 20.0f * baseScale);
    }

    // --- TYTUŁ ---
    float titleScale = 1.6f * baseScale; // Powiększony tytuł (wcześniej 1.2f)
    float titleW = Gui::MeasureTextWidth("SETTINGS", titleScale);
    Gui::DrawGuiText("SETTINGS", { panelPos.x + (boardSize.x - titleW) * 0.5f, panelPos.y + 80.0f * baseScale }, titleScale, { 1.0f, 1.0f, 1.0f, 1.0f }); // Zmieniony kolor na biały

    // --- LOGIKA MYSZKI I PRZYCISKÓW GRAFICZNYCH ---
    glm::vec2 mouse = Gui::GetMappedMousePos();
    auto isHov = [&](glm::vec2 p, glm::vec2 s) {
        return mouse.x >= p.x && mouse.x <= p.x + s.x && mouse.y >= p.y && mouse.y <= p.y + s.y;
        };

    static bool s_LastMouseStateSettings = false;
    bool currentMouseState = Input::IsMouseButtonPressed(0);
    bool mouseClicked = currentMouseState && !s_LastMouseStateSettings;

    // Funkcja lambdy rysująca grafikę przycisku (Z TINTEM I HOVEREM)
    auto drawImageBtn = [&](std::shared_ptr<Texture> tex, glm::vec2 basePos, glm::vec2 baseSize, float& scaleVar, bool hovered) {
        float targetScale = hovered ? 1.05f : 1.0f;
        scaleVar += (targetScale - scaleVar) * 15.0f * m_DeltaTime;

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
    float textScale = 1.0f * baseScale; // Powiększone napisy

    glm::vec2 arrowSize = { 50.0f * baseScale, 50.0f * baseScale };
    float distanceBetweenArrows = 300.0f * baseScale;

    // --- WYLICZENIE WSPÓLNEGO ŚRODKA DLA RZĘDU ---
    // Strzałki są centrowane względem swojego środka, napisy względem swojego top/baseline
    float arrowYOffset = arrowSize.y * 0.5f;
    float textYOffset = 18.0f * baseScale; // <-- ZMIENIAJ TĘ WARTOŚĆ, JEŚLI TEKST JEST MINIMALNIE ZA WYSOKO LUB ZA NISKO!

    // Funkcja wyśrodkowująca tekst idealnie między strzałkami < > oraz w pionie
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
        m_PendingMsaaIndex = (m_PendingMsaaIndex - 1 + m_MsaaOptions.size()) % m_MsaaOptions.size();
    }

    std::string msaaText = m_MsaaOptions[m_PendingMsaaIndex] == 1 ? "Off" : "MSAA x" + std::to_string(m_MsaaOptions[m_PendingMsaaIndex]);
    drawCenteredValue(msaaText, row2Y);

    glm::vec2 msaaRightPos = { rightColX + distanceBetweenArrows, row2Y - arrowYOffset };
    bool hovMsaaR = isHov(msaaRightPos, arrowSize);
    if (drawImageBtn(rightArrowTex, msaaRightPos, arrowSize, m_MsaaRightBtnScale, hovMsaaR)) {
        m_PendingMsaaIndex = (m_PendingMsaaIndex + 1) % m_MsaaOptions.size();
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
        SetVisible(false);
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
        int newMsaa = m_MsaaOptions[m_PendingMsaaIndex];

        if (gs.WindowWidth != newWidth || gs.WindowHeight != newHeight || gs.MsaaSamples != newMsaa) {
            gs.WindowWidth = newWidth;
            gs.WindowHeight = newHeight;
            gs.MsaaSamples = newMsaa;

            Application::Get().ApplyGraphicsSettings();
        }
    }

    // Zapis stanu myszki na koniec klatki (obsługa blokowania spamu kliknięć)
    s_LastMouseStateSettings = currentMouseState;
}