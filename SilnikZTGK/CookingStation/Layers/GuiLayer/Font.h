#pragma once
#include <map>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "CookingStation/Core/Texture.h"

struct Character {
    glm::vec2 UV_Min;
    glm::vec2 UV_Max;
    glm::vec2 Size;
    glm::vec2 Offset; 
    float Advance;
};

class Font {
public:
    Font(const std::string& fontPath, float fontSize);

    std::shared_ptr<Texture> GetTexture() { return m_Texture; }
    const Character& GetChar(char c) { return m_Characters[c]; }
    const std::map<char, Character>& GetCharacters() const { return m_Characters; }

private:
    std::shared_ptr<Texture> m_Texture;
    std::map<char, Character> m_Characters;
};