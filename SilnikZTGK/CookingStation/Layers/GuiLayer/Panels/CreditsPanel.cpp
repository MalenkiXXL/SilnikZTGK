#include "CreditsPanel.h"
#include "CookingStation/Layers/GuiLayer/Utils/Gui.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Layers/GuiLayer/Utils/Renderer2D.h"
#include "CookingStation/Layers/GuiLayer/Utils/AudioConfig.h"
#include <unordered_map>

void CreditsPanel::OnUpdate(float dt) {
    m_DeltaTime = dt;
}

void CreditsPanel::Draw(float baseScale) {
    if (!m_IsVisible) return;

    auto windowSize = Input::GetWindowSize();
    float screenW = (float)windowSize.first;
    float screenH = (float)windowSize.second;

    // To samo tlo co w SettingsMenuPanel - zachowujemy spojny styl UI.
    auto boardTex = AssetManager::GetTexture("assets://UI/cuttingBoard.png");
    auto backBtnTex = AssetManager::GetTexture("assets://UI/backButton.png");

    glm::vec2 uv0 = { 0.0f, 1.0f };
    glm::vec2 uv1 = { 1.0f, 0.0f };

    auto getAspectSize = [&](const std::shared_ptr<Texture>& tex, float targetHeight) -> glm::vec2 {
        if (tex && tex->GetRendererID() != 0) {
            float aspect = (float)tex->GetWidth() / (float)tex->GetHeight();
            return { targetHeight * aspect, targetHeight };
        }
        return { targetHeight * 3.0f, targetHeight };
        };

    float boardHeight = 1050.0f * baseScale;
    glm::vec2 boardSize = getAspectSize(boardTex, boardHeight);
    if (boardSize.x < 850.0f * baseScale) boardSize.x = 850.0f * baseScale;

    glm::vec2 panelPos = { (screenW - boardSize.x) * 0.5f, (screenH - boardSize.y) * 0.5f };

    if (boardTex && boardTex->GetRendererID() != 0) {
        Renderer2D::DrawQuad(panelPos, boardSize, boardTex->GetRendererID(), { 1.0f, 1.0f, 1.0f, 1.0f }, uv0, uv1);
    }
    else {
        Gui::Panel(panelPos, boardSize, { 0.12f, 0.12f, 0.15f, 0.95f }, 20.0f * baseScale);
    }

    // Wspolny kolor dla tekstu pomocniczego - troche ciemniejszy/cieplejszy
    // niz poprzedni jasny szary, zeby nie zlewal sie z jasnym tlem deski.
    glm::vec4 mutedTextColor = { 0.32f, 0.30f, 0.27f, 1.0f };

    // --- TYTUL ---
    float titleScale = 1.6f * baseScale;
    float titleW = Gui::MeasureTextWidth("CREDITS", titleScale);
    Gui::DrawGuiText("CREDITS", { panelPos.x + (boardSize.x - titleW) * 0.5f, panelPos.y + 80.0f * baseScale }, titleScale, { 1.0f, 1.0f, 1.0f, 1.0f });

    // Podtytul: nazwa gry + wersja (placeholder - podmienic na docelowe dane)
    std::string subtitleText = m_GameTitlePlaceholder + " - " + m_VersionPlaceholder;
    float subtitleScale = 0.6f * baseScale;
    float subtitleW = Gui::MeasureTextWidth(subtitleText, subtitleScale);
    Gui::DrawGuiText(subtitleText, { panelPos.x + (boardSize.x - subtitleW) * 0.5f, panelPos.y + 120.0f * baseScale }, subtitleScale, mutedTextColor);

    // --- LISTA TWORCOW ---
    float startY = panelPos.y + 200.0f * baseScale;
    float rowGap = 68.0f * baseScale;

    float nameScale = 0.85f * baseScale;
    float roleScale = 0.6f * baseScale;
    float roleOffsetY = 32.0f * baseScale;

    for (size_t i = 0; i < m_Credits.size(); i++) {
        float rowY = startY + rowGap * (float)i;
        const auto& entry = m_Credits[i];

        float nameW = Gui::MeasureTextWidth(entry.Name, nameScale);
        Gui::DrawGuiText(entry.Name, { panelPos.x + (boardSize.x - nameW) * 0.5f, rowY }, nameScale, { 1.0f, 1.0f, 1.0f, 1.0f });

        float roleW = Gui::MeasureTextWidth(entry.Role, roleScale);
        Gui::DrawGuiText(entry.Role, { panelPos.x + (boardSize.x - roleW) * 0.5f, rowY + roleOffsetY }, roleScale, mutedTextColor);
    }

    // --- STOPKA + DODATKOWE INFORMACJE (placeholder) ---
    // Prosty word-wrap, zeby dluzsze zdania ladnie mieScily sie na desce
    // niezaleznie od jej faktycznej szerokosci (zalezy od aspectu tekstury).
    float maxTextWidth = boardSize.x * 0.80f;
    auto wrapText = [&](const std::string& text, float scale) -> std::vector<std::string> {
        std::vector<std::string> lines;
        std::string currentLine;
        size_t pos = 0;
        while (pos < text.size()) {
            size_t spacePos = text.find(' ', pos);
            std::string word = text.substr(pos, spacePos == std::string::npos ? std::string::npos : spacePos - pos);
            std::string candidate = currentLine.empty() ? word : currentLine + " " + word;

            if (!currentLine.empty() && Gui::MeasureTextWidth(candidate, scale) > maxTextWidth) {
                lines.push_back(currentLine);
                currentLine = word;
            }
            else {
                currentLine = candidate;
            }

            if (spacePos == std::string::npos) break;
            pos = spacePos + 1;
        }
        if (!currentLine.empty()) lines.push_back(currentLine);
        return lines;
        };

    float bottomCursorY = startY + rowGap * (float)m_Credits.size() + 30.0f * baseScale;

    auto drawWrappedCentered = [&](const std::string& text, float scale, const glm::vec4& color, float lineGap) {
        for (const auto& line : wrapText(text, scale)) {
            float lineW = Gui::MeasureTextWidth(line, scale);
            Gui::DrawGuiText(line, { panelPos.x + (boardSize.x - lineW) * 0.5f, bottomCursorY }, scale, color);
            bottomCursorY += lineGap;
        }
        };

    float footerScale = 0.5f * baseScale;
    float lineGap = 27.0f * baseScale;

    drawWrappedCentered(m_FooterPlaceholder, footerScale, mutedTextColor, lineGap);

    bottomCursorY += 14.0f * baseScale; // odstep miedzy stopka a sekcja konkursowa

    drawWrappedCentered(m_CompetitionInfoPlaceholder, footerScale, mutedTextColor, lineGap);

    bottomCursorY += 14.0f * baseScale;

    drawWrappedCentered(m_FeedbackCallPlaceholder, footerScale, mutedTextColor, lineGap);

    bottomCursorY += 18.0f * baseScale;

    // "Contact info:" jako mini-naglowek tej sekcji - nieco wiekszy, ale wciaz
    // w tej samej, ciemniejszej tonacji co reszta tekstu pomocniczego.
    float contactLabelScale = 0.58f * baseScale;
    drawWrappedCentered(m_ContactLabelPlaceholder, contactLabelScale, mutedTextColor, lineGap);

    drawWrappedCentered(m_ContactDetailsPlaceholder, footerScale, mutedTextColor, lineGap);

    // --- INTERAKCJA / PRZYCISK BACK ---
    glm::vec2 mouse = Gui::GetMappedMousePos();
    auto isHov = [&](glm::vec2 p, glm::vec2 s) {
        return mouse.x >= p.x && mouse.x <= p.x + s.x && mouse.y >= p.y && mouse.y <= p.y + s.y;
        };

    static bool s_LastMouseStateCredits = false;
    bool currentMouseState = Input::IsMouseButtonPressed(0);
    bool mouseClicked = currentMouseState && !s_LastMouseStateCredits;

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

    float btnHeight = 90.0f * baseScale;
    glm::vec2 backBtnSize = getAspectSize(backBtnTex, btnHeight);
    float bottomY = panelPos.y + boardSize.y - btnHeight - 70.0f * baseScale;
    glm::vec2 backPos = { panelPos.x + (boardSize.x - backBtnSize.x) * 0.5f, bottomY };

    bool hovBack = isHov(backPos, backBtnSize);
    if (drawImageBtn(backBtnTex, backPos, backBtnSize, m_BackBtnScale, hovBack)) {
        SetVisible(false);
    }

    s_LastMouseStateCredits = currentMouseState;
}