#include "Gui.h"
#include "Renderer2D.h"
#include "CookingStation/Core/Input.h" 
#include <utility> // Dla std::pair

std::shared_ptr<Font> Gui::s_Font = nullptr;
float Gui::s_ScreenWidth = 800.0f;
float Gui::s_ScreenHeight = 600.0f;

bool Gui::IsMouseOver(const glm::vec2& pos, const glm::vec2& size) {
    // Pobieramy dane przez abstrakcjê Inputu
    auto mousePos = Input::GetMousePosition();
    auto windowSize = Input::GetWindowSize();

    float mouseX = mousePos.first * (s_ScreenWidth / windowSize.first);
    float mouseY = mousePos.second * (s_ScreenHeight / windowSize.second);

    return (mouseX >= pos.x && mouseX <= pos.x + size.x &&
        mouseY >= pos.y && mouseY <= pos.y + size.y);
}

glm::vec2 Gui::GetMappedMousePos() {
    auto mousePos = Input::GetMousePosition();
    auto windowSize = Input::GetWindowSize();

    float mouseX = mousePos.first * (s_ScreenWidth / windowSize.first);
    float mouseY = mousePos.second * (s_ScreenHeight / windowSize.second);
    return { mouseX, mouseY };
}

bool Gui::SliderFloat(const std::string& label, float* value, float min, float max, const glm::vec2& pos, const glm::vec2& size) {
    bool changed = false;

    // 0 = lewy przycisk myszy
   if (Input::IsMouseButtonPressed(0)) {
        if (IsMouseOver(pos, size)) {
            //Pobieramy przeliczon¹ pozycjê myszy zamiast surowej
            glm::vec2 mappedMouse = GetMappedMousePos();

            // Obliczamy procentowe przesuniêcie na podstawie przeskalowanych wspó³rzêdnych
            float t = (mappedMouse.x - pos.x) / size.x;

            *value = min + t * (max - min);

            if (*value < min) *value = min;
            if (*value > max) *value = max;

            changed = true;
        }
    }

    DrawGuiText(label, { pos.x, pos.y - 15.0f }, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });
   
    // Rysowanie tla
    Renderer2D::DrawQuad(pos, size, { 0.2f, 0.2f, 0.2f, 1.0f });

    // Obliczanie pozycji uchwytu
    float handleWidth = 10.0f;
    float handlePos = ((*value - min) / (max - min)) * size.x;

    // Rysowanie uchwytu
    Renderer2D::DrawQuad({ pos.x + handlePos - (handleWidth / 2.0f), pos.y }, { handleWidth, size.y }, { 0.8f, 0.8f, 0.8f, 1.0f });

    return changed;
}

void Gui::DrawGuiText(const std::string& text, glm::vec2 pos, float scale, const glm::vec4& color) {
    for (char c : text) {
        if (s_Font->GetCharacters().find(c) != s_Font->GetCharacters().end()) {
            auto& ch = s_Font->GetChar(c);

            glm::vec2 size = { ch.Size.x * scale, ch.Size.y * scale };

            // U¿ywamy rozbudowanego DrawQuad z UV
            Renderer2D::DrawQuad(pos, size, s_Font->GetTexture(), color, ch.UV_Min, ch.UV_Max);

            pos.x += ch.Advance * scale; // Przesuniêcie kursora
        }
    }
}

void Gui::Init(const std::string& fontPath, float fontSize) {
    s_Font = std::make_shared<Font>(fontPath, fontSize);
}

bool Gui::Button(const std::string & label, glm::vec2 & pos, glm::vec2 & size) {
    bool hovered = IsMouseOver(pos, size);
    bool clicked = false;

    glm::vec4 color = hovered ? glm::vec4(0.4f,0.4f,0.4f, 1.0f) : glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);

    if (hovered && Input::IsMouseButtonPressed(0)) {
        color = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        clicked = true;
    }

    Renderer2D::DrawQuad(pos, size, color);
    DrawGuiText(label, { pos.x + 10.f, pos.y + 5.f }, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });
    
    return clicked;
}

