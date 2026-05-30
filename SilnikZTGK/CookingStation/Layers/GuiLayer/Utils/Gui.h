#pragma once
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include <unordered_map>
#include "Font.h"
#include "CookingStation/Core/Texture.h"

class Gui {
public:
    static void Init(const std::string& fontPath, float fontSize);

    static void UpdateDeltaTime(float dt) { s_DeltaTime = dt; }
    static bool SliderFloat(const std::string& label, float* value, float min, float max, const glm::vec2& pos, const glm::vec2& size);
    static void DrawGuiText(const std::string& text, glm::vec2 pos, float scale, const glm::vec4& color);
    static bool Button(const std::string& label, const glm::vec2& pos, const glm::vec2& size, bool isActive = false);
    static void UpdateScreenSize(float width, float height) { s_ScreenWidth = width; s_ScreenHeight = height; }
    static bool InputGuiText(const std::string& label, std::string& value, const glm::vec2& pos, const glm::vec2& size);
    static glm::vec2 GetMappedMousePos();
    static bool IsMouseOver(const glm::vec2& pos, const glm::vec2& size);
    static void OnCharTyped(int charcode);
    static bool AnyItemActive() { return !s_ActiveWidgetID.empty(); }
    static void EndFrame() { s_CharacterBuffer.clear(); }
    static bool DragFloat(const std::string& label, float* value, float dragSpeed, const glm::vec2& pos, const glm::vec2& size);
    static void BeginFrame();
    static bool WantCaptureMouse() { return s_WantCaptureMouse; }
    static void Panel(const glm::vec2& pos, const glm::vec2& size, const glm::vec4& color, float radius = 15.0f);
    static void SetScreenSize(float width, float height) { s_ScreenWidth = width; s_ScreenHeight = height; }
    static float MeasureTextWidth(const std::string& text, float scale);
    static float MeasureTextHeight(const std::string& text, float scale);

    // Uniwersalny animowany przycisk
    static bool ScaledButton(const std::string& label,
        glm::vec2 basePos, glm::vec2 baseSize,
        float btnScale, float bsc,
        glm::vec4 colorNormal, glm::vec4 colorHover,
        bool hovered);

    // Uniwersalny sprężysty obrazek/ikona UI
    static bool BubblyImage(const std::string& id,
        const std::shared_ptr<Texture>& icon,
        glm::vec2 basePos, glm::vec2 baseSize,
        float dt, float hoverScale,
        bool darkenOnHover, float hitRadiusMultiplier = 0.5f,
        glm::vec4 tintColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        bool* outIsHovered = nullptr);

private:
    static std::shared_ptr<Font> s_Font;
    static float s_ScreenWidth;
    static float s_ScreenHeight;
    static std::string s_ActiveWidgetID;
    static std::string s_CharacterBuffer;
    static float s_DeltaTime;
    static bool s_WantCaptureMouse;
};