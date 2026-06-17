#pragma once
#include "CookingStation/Core/Texture.h"
#include "../Utils/BubblyUI.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>

class RecipeBookPanel {
public:
    void Init();
    void Draw(float gameX, float gameY, float gameW, float gameH, float baseScale, float dt, bool isGamePaused);

    bool IsOpen() const { return m_IsOpen; }
    void Close() { m_IsOpen = false; }

private:
    void DrawRecipeIcon(const std::string& recipeId, const std::shared_ptr<Texture>& texture,
        glm::vec2 relativePct, float targetHeight, glm::vec2 bookPos, glm::vec2 bookSize, float dt, bool isBlocked);

    bool m_IsOpen = false;
    std::unordered_map<std::string, BubblyState> m_BubblyStates;

    std::shared_ptr<Texture> m_BookCloudIcon;
    std::shared_ptr<Texture> m_BookIcon;
    std::shared_ptr<Texture> m_BookStarsIcon;
    std::shared_ptr<Texture> m_BookInsideIcon;
    std::shared_ptr<Texture> m_BookXIcon;
    std::shared_ptr<Texture> m_TomatoSoupIcon;
    std::shared_ptr<Texture> m_SandwichIcon;
    std::shared_ptr<Texture> m_CupcakeIcon;
    std::shared_ptr<Texture> m_CroissantIcon;
};