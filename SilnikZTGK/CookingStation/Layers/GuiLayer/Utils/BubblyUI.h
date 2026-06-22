#pragma once
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Layers/GuiLayer/Utils/Renderer2D.h"
#include "CookingStation/Layers/GuiLayer/Utils/Gui.h"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <memory>
#include <algorithm>

struct BubblyState {
    float scale = 1.0f;
    glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool wasHovered = false;
};

class BubblyUI {
public:
    static bool DrawBubblyImage(std::unordered_map<std::string, BubblyState>& states,
        const std::string& id, const std::shared_ptr<Texture>& icon,
        glm::vec2 basePos, glm::vec2 baseSize, float dt, bool isBlocked,
        float hoverScale = 1.15f, bool darkenOnHover = false, float hitRadiusMultiplier = 0.5f,
        glm::vec4 tintColor = { 1.0f, 1.0f, 1.0f, 1.0f }, bool* outIsHovered = nullptr)
    {
        if (!icon) return false;
        auto& state = states[id];
        glm::vec2 mousePos = Gui::GetMappedMousePos();

        if (isBlocked) mousePos = glm::vec2(-10000.0f, -10000.0f);

        float animSpeed = 15.0f;
        glm::vec2 center = { basePos.x + baseSize.x * 0.5f, basePos.y + baseSize.y * 0.5f };
        float hitRadius = std::min(baseSize.x, baseSize.y) * hitRadiusMultiplier * state.scale;
        float distX = mousePos.x - center.x;
        float distY = mousePos.y - center.y;
        bool isHovered = (distX * distX + distY * distY) <= (hitRadius * hitRadius);

        if (isHovered && !state.wasHovered) {
            AudioEngine::PlayLoopingSound("assets://sounds/hover_in_game.mp3", 0.15f, false);
            state.wasHovered = true;
        } else if (!isHovered) {
            state.wasHovered = false;
        }

        if (outIsHovered != nullptr) *outIsHovered = isHovered;
        if (isHovered) Input::SetUICaptureMouse(true);

        float targetScale = isHovered ? hoverScale : 1.0f;
        glm::vec4 targetColor = (isHovered && darkenOnHover) ? tintColor * glm::vec4(0.8f, 0.8f, 0.8f, 1.0f) : tintColor;

        state.scale += (targetScale - state.scale) * dt * animSpeed;
        state.color.r += (targetColor.r - state.color.r) * dt * animSpeed;
        state.color.g += (targetColor.g - state.color.g) * dt * animSpeed;
        state.color.b += (targetColor.b - state.color.b) * dt * animSpeed;

        glm::vec2 size = baseSize * state.scale;
        glm::vec2 pos = { basePos.x + (baseSize.x * 0.5f) - (size.x * 0.5f), basePos.y + (baseSize.y * 0.5f) - (size.y * 0.5f) };

        if (id == "CloudRight") Renderer2D::DrawQuad(pos, size, icon, state.color, { 1.0f, 1.0f }, { 0.0f, 0.0f });
        else Renderer2D::DrawQuad(pos, size, icon, state.color, { 0.0f, 1.0f }, { 1.0f, 0.0f });

        return (Input::IsMouseButtonJustPressed(0) && isHovered);
    }
};