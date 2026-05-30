#pragma once
#include <string>
#include <glm/glm.hpp>
#include <memory>
#include "CookingStation/Core/Texture.h"

namespace GuiUtils {

    enum class Anchor { TopLeft, TopRight, BottomLeft, BottomRight, Center };

    glm::vec2 GetAnchoredPosition(Anchor anchor, float offsetX, float offsetY, float width, float height, float screenWidth, float screenHeight);

    void DrawWrappedGuiText(const std::string& text, glm::vec2 startPos, float scale, const glm::vec4& color, float lineSpacing = 24.0f, size_t maxCharsPerLine = 36);

    bool DrawScaledButton(const std::string& label, glm::vec2 basePos, glm::vec2 baseSize, float& currentScale, float targetScaleMultiplier, float baseScale, glm::vec4 colorNormal, glm::vec4 colorHover, bool hovered, float dt);

    glm::vec2 CalculateAspectSize(const std::shared_ptr<Texture>& texture, float targetHeight);
}