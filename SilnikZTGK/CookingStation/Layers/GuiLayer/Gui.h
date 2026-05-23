#pragma once
#include <glm/glm.hpp>
#include <string>
#include <CookingStation/Layers/GuiLayer/Font.h>

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


private:
    static std::shared_ptr<Font> s_Font;
    static float s_ScreenWidth;
    static float s_ScreenHeight;
    static std::string s_CharacterBuffer;
    static std::string s_ActiveWidgetID;
    static float s_DeltaTime;
    static bool s_WantCaptureMouse;
};