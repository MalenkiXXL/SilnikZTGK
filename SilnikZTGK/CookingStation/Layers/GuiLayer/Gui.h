#pragma once
#include <glm/glm.hpp>
#include <string>
#include <CookingStation/Layers/GuiLayer/Font.h>

class Gui {
public:
    static void Init(const std::string& fontPath, float fontSize);
    // Zwraca true, jeœli wartoœæ zosta³a zmieniona
    static bool SliderFloat(const std::string& label, float* value, float min, float max, const glm::vec2& pos, const glm::vec2& size);
    static void DrawGuiText(const std::string& text, glm::vec2 pos, float scale, const glm::vec4& color);
    static bool Button(const std::string& label, glm::vec2& pos, glm::vec2& size);
    static void UpdateScreenSize(float width, float height) { s_ScreenWidth = width; s_ScreenHeight = height; }

private:
    // Pomocnicza funkcja do sprawdzania, czy mysz jest nad prostok¹tem
    static bool IsMouseOver(const glm::vec2& pos, const glm::vec2& size);
    static glm::vec2 GetMappedMousePos();
    static std::shared_ptr<Font> s_Font;
    static float s_ScreenWidth;
    static float s_ScreenHeight;
};