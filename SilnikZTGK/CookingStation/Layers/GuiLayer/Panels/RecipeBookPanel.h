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
    void DrawRecipeIcon(const std::string& recipeId, const std::string& displayName, const std::shared_ptr<Texture>& iconTex, const std::shared_ptr<Texture>& tooltipTex,
        glm::vec2 relativePct, float targetWidth, glm::vec2 bookPos, glm::vec2 bookSize, float dt, bool isBlocked,
        std::shared_ptr<Texture>& outTooltipTex, glm::vec2& outTooltipPos, glm::vec2& outTooltipSize);

    bool m_IsOpen = false;
    std::unordered_map<std::string, BubblyState> m_BubblyStates;

    std::shared_ptr<Texture> m_BookCloudIcon;
    std::shared_ptr<Texture> m_BookIcon;
    std::shared_ptr<Texture> m_BookStarsIcon;
    std::shared_ptr<Texture> m_BookInsideIcon;
    std::shared_ptr<Texture> m_BookXIcon;

    // --- IKONKI DAÑ ---
    std::shared_ptr<Texture> m_TomatoSoupIcon;
    std::shared_ptr<Texture> m_SandwichIcon;
    std::shared_ptr<Texture> m_FriedEggIcon;
    std::shared_ptr<Texture> m_EggsAndBaconIcon;
    std::shared_ptr<Texture> m_ShakshukaIcon;
    std::shared_ptr<Texture> m_BaguetteIcon;

    // --- TEKSTURY PRZEPISÓW (Chmurki) ---
    std::shared_ptr<Texture> m_TomatoSoupRecipeTex;
    std::shared_ptr<Texture> m_SandwichRecipeTex;
    std::shared_ptr<Texture> m_FriedEggRecipeTex;
    std::shared_ptr<Texture> m_EggsAndBaconRecipeTex;
    std::shared_ptr<Texture> m_ShakshukaRecipeTex;
    std::shared_ptr<Texture> m_BaguetteRecipeTex;
};