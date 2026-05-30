#include "GuiUtils.h"
#include "Gui.h"
#include <sstream>
#include <algorithm>

namespace GuiUtils {

    glm::vec2 GetAnchoredPosition(Anchor anchor, float offsetX, float offsetY, float width, float height, float screenWidth, float screenHeight) {
        switch (anchor) {
        case Anchor::TopLeft:     return { offsetX, offsetY };
        case Anchor::TopRight:    return { screenWidth - width - offsetX, offsetY };
        case Anchor::BottomLeft:  return { offsetX, screenHeight - height - offsetY };
        case Anchor::BottomRight: return { screenWidth - width - offsetX, screenHeight - height - offsetY };
        case Anchor::Center:      return { (screenWidth - width) / 2.0f + offsetX, (screenHeight - height) / 2.0f + offsetY };
        }
        return { offsetX, offsetY };
    }

    void DrawWrappedGuiText(const std::string& text, glm::vec2 startPos, float scale, const glm::vec4& color, float lineSpacing, size_t maxCharsPerLine) {
        std::stringstream ss(text);
        std::string word;
        std::string currentLine = "";
        glm::vec2 currentPos = startPos;

        while (ss >> word) {
            if (currentLine.length() + word.length() + 1 > maxCharsPerLine) {
                if (!currentLine.empty()) {
                    // Cień
                    Gui::DrawGuiText(currentLine, { currentPos.x + 2.0f, currentPos.y + 2.0f }, scale, { 0.0f, 0.0f, 0.0f, 0.85f });
                    // Właściwy tekst
                    Gui::DrawGuiText(currentLine, currentPos, scale, color);
                    currentPos.y += lineSpacing;
                }
                currentLine = word;
            }
            else {
                if (!currentLine.empty()) currentLine += " ";
                currentLine += word;
            }
        }
        if (!currentLine.empty()) {
            Gui::DrawGuiText(currentLine, { currentPos.x + 2.0f, currentPos.y + 2.0f }, scale, { 0.0f, 0.0f, 0.0f, 0.85f });
            Gui::DrawGuiText(currentLine, currentPos, scale, color);
        }
    }

    bool DrawScaledButton(const std::string& label, glm::vec2 basePos, glm::vec2 baseSize, float& currentScale, float targetScaleMultiplier, float baseScale, glm::vec4 colorNormal, glm::vec4 colorHover, bool hovered, float dt) {
        float animSpeed = 15.0f;
        // Płynna interpolacja skali
        currentScale += (targetScaleMultiplier - currentScale) * dt * animSpeed;

        // Zwraca true jeśli kliknięto
        return Gui::ScaledButton(label, basePos, baseSize, currentScale, baseScale, colorNormal, colorHover, hovered);
    }

    glm::vec2 CalculateAspectSize(const std::shared_ptr<Texture>& texture, float targetHeight) {
        if (!texture) return { targetHeight, targetHeight };
        float aspect = (float)texture->GetWidth() / (float)texture->GetHeight();
        return { targetHeight * aspect, targetHeight };
    }

}