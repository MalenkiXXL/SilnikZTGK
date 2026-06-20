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
#include <GLFW/glfw3.h> // Wymagane do sprawdzenia rozdzielczości monitora
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Layers/GuiLayer/Utils/AudioConfig.h"

SettingsMenuPanel::SettingsMenuPanel() {
    SyncWithEngine();
}

void SettingsMenuPanel::SyncWithEngine() {
    auto& gs = GraphicsSettings::Get();

    // Sprawdzamy natywną rozdzielczość monitora użytkownika
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    int monitorWidth = mode->width;
    int monitorHeight = mode->height;

    // Szukamy największego indeksu, który mieści się na tym ekranie
    m_MaxResIndex = 0;
    for (int i = 0; i < GraphicsSettings::ResolutionCount; i++) {
        if (GraphicsSettings::Resolutions[i].first <= monitorWidth &&
            GraphicsSettings::Resolutions[i].second <= monitorHeight) {
            m_MaxResIndex = i;
        }
    }

    for (int i = 0; i < GraphicsSettings::ResolutionCount; i++) {
        if (GraphicsSettings::Resolutions[i].first == gs.WindowWidth &&
            GraphicsSettings::Resolutions[i].second == gs.WindowHeight) {
            m_PendingResIndex = i;
            break;
        }
    }

    // Blokada przed wejściem wyżej niż pozwala monitor
    if (m_PendingResIndex > m_MaxResIndex) {
        m_PendingResIndex = m_MaxResIndex;
    }

    for (size_t i = 0; i < m_MsaaOptions.size(); i++) {
        if (m_MsaaOptions[i] == gs.MsaaSamples) {
            m_PendingMsaaIndex = (int)i;
            break;
        }
    }

    m_PendingMusicEnabled = AudioEngine::IsMusicEnabled();
    m_PendingSoundsEnabled = AudioEngine::AreSoundsEnabled();
    m_PendingFullscreen = gs.Fullscreen;
}

void SettingsMenuPanel::OnUpdate(float dt) {
    m_DeltaTime = dt;
}

void SettingsMenuPanel::Draw(float baseScale) {
    if (!m_IsVisible) return;

    auto windowSize = Input::GetWindowSize();
    float screenW = (float)windowSize.first;
    float screenH = (float)windowSize.second;

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

    float boardHeight = 850.0f * baseScale;
    glm::vec2 boardSize = getAspectSize(boardTex, boardHeight);
    if (boardSize.x < 850.0f * baseScale) boardSize.x = 850.0f * baseScale;

    glm::vec2 panelPos = { (screenW - boardSize.x) * 0.5f, (screenH - boardSize.y) * 0.5f };

    if (boardTex && boardTex->GetRendererID() != 0) {
        Renderer2D::DrawQuad(panelPos, boardSize, boardTex->GetRendererID(), { 1.0f, 1.0f, 1.0f, 1.0f }, uv0, uv1);
    }
    else {
        Gui::Panel(panelPos, boardSize, { 0.12f, 0.12f, 0.15f, 0.95f }, 20.0f * baseScale);
    }

    float titleScale = 1.6f * baseScale;
    float titleW = Gui::MeasureTextWidth("SETTINGS", titleScale);
    Gui::DrawGuiText("SETTINGS", { panelPos.x + (boardSize.x - titleW) * 0.5f, panelPos.y + 80.0f * baseScale }, titleScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    glm::vec2 mouse = Gui::GetMappedMousePos();
    auto isHov = [&](glm::vec2 p, glm::vec2 s) {
        return mouse.x >= p.x && mouse.x <= p.x + s.x && mouse.y >= p.y && mouse.y <= p.y + s.y;
        };

    static bool s_LastMouseStateSettings = false;
    bool currentMouseState = Input::IsMouseButtonPressed(0);
    bool mouseClicked = currentMouseState && !s_LastMouseStateSettings;

    auto drawImageBtn = [&](std::shared_ptr<Texture> tex, glm::vec2 basePos, glm::vec2 baseSize, float& scaleVar, bool hovered, bool active = true) {
        if (!active) {
            if (tex && tex->GetRendererID() != 0) {
                Renderer2D::DrawQuad(basePos, baseSize, tex->GetRendererID(), { 0.5f, 0.5f, 0.5f, 0.3f }, uv0, uv1);
            }
            else {
                Gui::Panel(basePos, baseSize, { 0.5f, 0.5f, 0.5f, 0.3f }, 10.0f * baseScale);
            }
            return false;
        }

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

        static std::unordered_map<float*, bool> lastHoverStates;

        bool wasHoveredLastFrame = lastHoverStates[&scaleVar];

        if (hovered && !wasHoveredLastFrame) {
            AudioEngine::Play(AudioConfig::ButtonHoverSound);
        }

        lastHoverStates[&scaleVar] = hovered;

        bool isClicked = hovered && mouseClicked;
        if (isClicked) {
            AudioEngine::Play(AudioConfig::ButtonClickSound);
        }

        return isClicked;
        };

    float startY = panelPos.y + 180.0f * baseScale;
    float rowGap = 85.0f * baseScale;

    float leftColX = panelPos.x + boardSize.x * 0.15f;
    float rightColX = panelPos.x + boardSize.x * 0.48f;
    float textScale = 1.0f * baseScale;

    glm::vec2 arrowSize = { 50.0f * baseScale, 50.0f * baseScale };
    float distanceBetweenArrows = 300.0f * baseScale;
    float arrowYOffset = arrowSize.y * 0.5f;
    float textYOffset = 18.0f * baseScale;

    auto drawCenteredValue = [&](const std::string& text, float rowCenterY) {
        float textW = Gui::MeasureTextWidth(text, textScale);
        float centerX = rightColX + arrowSize.x + (distanceBetweenArrows - arrowSize.x) * 0.5f;
        Gui::DrawGuiText(text, { centerX - textW * 0.5f, rowCenterY - textYOffset }, textScale, { 1.0f, 1.0f, 1.0f, 1.0f });
        };

    // --- RZĄD 1: ROZDZIELCZOŚĆ ---
    float row1Y = startY;
    Gui::DrawGuiText("Resolution:", { leftColX, row1Y - textYOffset }, textScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    glm::vec2 resLeftPos = { rightColX, row1Y - arrowYOffset };
    bool canGoLeft = m_PendingResIndex > 0;
    bool hovResL = isHov(resLeftPos, arrowSize) && canGoLeft;
    if (drawImageBtn(leftArrowTex, resLeftPos, arrowSize, m_ResLeftBtnScale, hovResL, canGoLeft)) {
        m_PendingResIndex--;
    }

    std::string resText = std::to_string(GraphicsSettings::Resolutions[m_PendingResIndex].first) + " x " + std::to_string(GraphicsSettings::Resolutions[m_PendingResIndex].second);
    drawCenteredValue(resText, row1Y);

    glm::vec2 resRightPos = { rightColX + distanceBetweenArrows, row1Y - arrowYOffset };
    bool canGoRight = m_PendingResIndex < m_MaxResIndex;
    bool hovResR = isHov(resRightPos, arrowSize) && canGoRight;
    if (drawImageBtn(rightArrowTex, resRightPos, arrowSize, m_ResRightBtnScale, hovResR, canGoRight)) {
        m_PendingResIndex++;
    }

    // --- RZĄD 2: ANTI-ALIASING ---
    float row2Y = startY + rowGap;
    Gui::DrawGuiText("Antialiasing:", { leftColX, row2Y - textYOffset }, textScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    glm::vec2 msaaLeftPos = { rightColX, row2Y - arrowYOffset };
    bool canMsaaGoLeft = m_PendingMsaaIndex > 0;
    bool hovMsaaL = isHov(msaaLeftPos, arrowSize) && canMsaaGoLeft;
    if (drawImageBtn(leftArrowTex, msaaLeftPos, arrowSize, m_MsaaLeftBtnScale, hovMsaaL, canMsaaGoLeft)) {
        m_PendingMsaaIndex--;
    }

    std::string msaaText = m_MsaaOptions[m_PendingMsaaIndex] == 1 ? "Off" : "MSAA x" + std::to_string(m_MsaaOptions[m_PendingMsaaIndex]);
    drawCenteredValue(msaaText, row2Y);

    glm::vec2 msaaRightPos = { rightColX + distanceBetweenArrows, row2Y - arrowYOffset };
    bool canMsaaGoRight = m_PendingMsaaIndex < (int)m_MsaaOptions.size() - 1;
    bool hovMsaaR = isHov(msaaRightPos, arrowSize) && canMsaaGoRight;
    if (drawImageBtn(rightArrowTex, msaaRightPos, arrowSize, m_MsaaRightBtnScale, hovMsaaR, canMsaaGoRight)) {
        m_PendingMsaaIndex++;
    }

    // --- RZĄD 3: FULLSCREEN ---
    float row3Y = startY + rowGap * 2.0f;
    Gui::DrawGuiText("Fullscreen:", { leftColX, row3Y - textYOffset }, textScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    glm::vec2 fsLeftPos = { rightColX, row3Y - arrowYOffset };
    bool canFsGoLeft = m_PendingFullscreen == true; // Lewa strzałka = chcemy wyłączyć (czyli musi być aktualnie włączony)
    bool hovFsL = isHov(fsLeftPos, arrowSize) && canFsGoLeft;
    if (drawImageBtn(leftArrowTex, fsLeftPos, arrowSize, m_FsLeftBtnScale, hovFsL, canFsGoLeft)) {
        m_PendingFullscreen = false;
    }

    std::string fsText = m_PendingFullscreen ? "ON" : "OFF";
    drawCenteredValue(fsText, row3Y);

    glm::vec2 fsRightPos = { rightColX + distanceBetweenArrows, row3Y - arrowYOffset };
    bool canFsGoRight = m_PendingFullscreen == false; // Prawa strzałka = chcemy włączyć
    bool hovFsR = isHov(fsRightPos, arrowSize) && canFsGoRight;
    if (drawImageBtn(rightArrowTex, fsRightPos, arrowSize, m_FsRightBtnScale, hovFsR, canFsGoRight)) {
        m_PendingFullscreen = true;
    }

    // --- RZĄD 4: MUZYKA ---
    float row4Y = startY + rowGap * 3.0f;
    Gui::DrawGuiText("Music:", { leftColX, row4Y - textYOffset }, textScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    glm::vec2 musLeftPos = { rightColX, row4Y - arrowYOffset };
    bool canMusGoLeft = m_PendingMusicEnabled == true;
    bool hovMusL = isHov(musLeftPos, arrowSize) && canMusGoLeft;
    if (drawImageBtn(leftArrowTex, musLeftPos, arrowSize, m_MusicLeftBtnScale, hovMusL, canMusGoLeft)) {
        m_PendingMusicEnabled = false;
    }

    std::string musText = m_PendingMusicEnabled ? "ON" : "OFF";
    drawCenteredValue(musText, row4Y);

    glm::vec2 musRightPos = { rightColX + distanceBetweenArrows, row4Y - arrowYOffset };
    bool canMusGoRight = m_PendingMusicEnabled == false;
    bool hovMusR = isHov(musRightPos, arrowSize) && canMusGoRight;
    if (drawImageBtn(rightArrowTex, musRightPos, arrowSize, m_MusicRightBtnScale, hovMusR, canMusGoRight)) {
        m_PendingMusicEnabled = true;
    }

    // --- RZĄD 5: DŹWIĘKI ---
    float row5Y = startY + rowGap * 4.0f;
    Gui::DrawGuiText("Sounds:", { leftColX, row5Y - textYOffset }, textScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    glm::vec2 sndLeftPos = { rightColX, row5Y - arrowYOffset };
    bool canSndGoLeft = m_PendingSoundsEnabled == true;
    bool hovSndL = isHov(sndLeftPos, arrowSize) && canSndGoLeft;
    if (drawImageBtn(leftArrowTex, sndLeftPos, arrowSize, m_SoundsLeftBtnScale, hovSndL, canSndGoLeft)) {
        m_PendingSoundsEnabled = false;
    }

    std::string sndText = m_PendingSoundsEnabled ? "ON" : "OFF";
    drawCenteredValue(sndText, row5Y);

    glm::vec2 sndRightPos = { rightColX + distanceBetweenArrows, row5Y - arrowYOffset };
    bool canSndGoRight = m_PendingSoundsEnabled == false;
    bool hovSndR = isHov(sndRightPos, arrowSize) && canSndGoRight;
    if (drawImageBtn(rightArrowTex, sndRightPos, arrowSize, m_SoundsRightBtnScale, hovSndR, canSndGoRight)) {
        m_PendingSoundsEnabled = true;
    }

    // --- DOLNY PANEL ---
    float btnHeight = 90.0f * baseScale;
    glm::vec2 backBtnSize = getAspectSize(backBtnTex, btnHeight);
    glm::vec2 applyBtnSize = getAspectSize(applyBtnTex, btnHeight);
    float bottomY = panelPos.y + boardSize.y - btnHeight - 70.0f * baseScale;

    glm::vec2 backPos = { panelPos.x + boardSize.x * 0.12f, bottomY };
    bool hovBack = isHov(backPos, backBtnSize);
    if (drawImageBtn(backBtnTex, backPos, backBtnSize, m_BackBtnScale, hovBack)) {
        SetVisible(false);
    }

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

        if (gs.WindowWidth != newWidth || gs.WindowHeight != newHeight || gs.MsaaSamples != newMsaa || gs.Fullscreen != m_PendingFullscreen) {
            gs.WindowWidth = newWidth;
            gs.WindowHeight = newHeight;
            gs.MsaaSamples = newMsaa;
            gs.Fullscreen = m_PendingFullscreen;

            Application::Get().ApplyGraphicsSettings();
        }
    }

    s_LastMouseStateSettings = currentMouseState;
}