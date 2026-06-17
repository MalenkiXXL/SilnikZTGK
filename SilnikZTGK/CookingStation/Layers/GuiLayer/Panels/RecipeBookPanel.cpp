#include "RecipeBookPanel.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Layers/GuiLayer/Utils/GuiUtils.h"
#include "CookingStation/Core/GameProgress.h"

void RecipeBookPanel::Init() {
    m_BookCloudIcon = AssetManager::GetTexture("assets://UI/bookCloud.png");
    m_BookIcon = AssetManager::GetTexture("assets://UI/book.png");
    m_BookStarsIcon = AssetManager::GetTexture("assets://UI/bookStars.png");
    m_BookInsideIcon = AssetManager::GetTexture("assets://UI/bookInside.png");
    m_BookXIcon = AssetManager::GetTexture("assets://UI/bookX.png");
    m_TomatoSoupIcon = AssetManager::GetTexture("assets://UI/tomatoSoup.png");
    m_SandwichIcon = AssetManager::GetTexture("assets://UI/sandwich.png");
    m_CupcakeIcon = AssetManager::GetTexture("assets://UI/cupcake.png");
    m_CroissantIcon = AssetManager::GetTexture("assets://UI/croissant.png");
}

void RecipeBookPanel::DrawRecipeIcon(const std::string& recipeId, const std::shared_ptr<Texture>& texture,
    glm::vec2 relativePct, float targetHeight, glm::vec2 bookPos, glm::vec2 bookSize, float dt, bool isBlocked)
{
    if (!texture) return;
    glm::vec2 size = GuiUtils::CalculateAspectSize(texture, targetHeight);
    glm::vec2 pos = { bookPos.x + bookSize.x * relativePct.x, bookPos.y + bookSize.y * relativePct.y };
    bool isUnlocked = GameProgress::IsRecipeUnlocked(recipeId);
    glm::vec4 tint = isUnlocked ? glm::vec4(1.0f) : glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);
    BubblyUI::DrawBubblyImage(m_BubblyStates, "Recipe_" + recipeId, texture, pos, size, dt, isBlocked, 1.15f, true, 0.5f, tint);
}

void RecipeBookPanel::Draw(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt, bool isGamePaused) {
    if (!m_BookIcon) return;

    bool isBlocked = isGamePaused;
    glm::vec2 cloudSize = { 210.0f * baseScale, 210.0f * baseScale };
    glm::vec2 cloudPos = { gameX + 10.0f * baseScale, gameY * baseScale };
    glm::vec2 actualCloudSize = cloudSize * 1.3f;

    if (m_BookCloudIcon) BubblyUI::DrawBubblyImage(m_BubblyStates, "BookCloud", m_BookCloudIcon, cloudPos, actualCloudSize, dt, false, 1.1f, false);

    if (!m_IsOpen) {
        glm::vec2 bookSize = cloudSize * 1.1f;
        glm::vec2 bookPos = { cloudPos.x + (actualCloudSize.x - bookSize.x) * 0.5f, cloudPos.y + (actualCloudSize.y - bookSize.y) * 0.5f };

        if (BubblyUI::DrawBubblyImage(m_BubblyStates, "BookIcon", m_BookIcon, bookPos, bookSize, dt, isBlocked, 1.15f, true, 0.35f)) m_IsOpen = true;
        if (m_BookStarsIcon) BubblyUI::DrawBubblyImage(m_BubblyStates, "BookStars", m_BookStarsIcon, cloudPos, actualCloudSize, dt, isBlocked, 1.15f, false);
    }
    else {
        glm::vec2 insideSize = GuiUtils::CalculateAspectSize(m_BookInsideIcon, gameHeight * 1.0f);
        float yOffset = 50.0f * baseScale;
        glm::vec2 insidePos = { gameX + (gameWidth - insideSize.x) * 0.5f, gameY + (gameHeight - insideSize.y) * 0.5f + yOffset };

        if (m_BookInsideIcon) BubblyUI::DrawBubblyImage(m_BubblyStates, "BookInside", m_BookInsideIcon, insidePos, insideSize, dt, false, 1.0f, false);

        glm::vec2 xSize = { 60.0f * baseScale, 60.0f * baseScale };
        glm::vec2 xPos = { insidePos.x + insideSize.x - xSize.x * 2.6f, insidePos.y + xSize.y * 2.6f };

        if (m_BookXIcon) {
            if (BubblyUI::DrawBubblyImage(m_BubblyStates, "BookX", m_BookXIcon, xPos, xSize, dt, false, 1.2f, true, 0.4f)) m_IsOpen = false;
        }

        float recipeH = 120.0f * baseScale;
        DrawRecipeIcon("TomatoSoup", m_TomatoSoupIcon, { 0.12f, 0.15f }, recipeH, insidePos, insideSize, dt, false);
        DrawRecipeIcon("Sandwich", m_SandwichIcon, { 0.35f, 0.15f }, recipeH, insidePos, insideSize, dt, false);
        DrawRecipeIcon("Croissant", m_CroissantIcon, { 0.12f, 0.30f }, recipeH - 20.0f, insidePos + glm::vec2(10.0f), insideSize, dt, false);
        DrawRecipeIcon("Cupcake", m_CupcakeIcon, { 0.35f, 0.30f }, recipeH, insidePos, insideSize, dt, false);
    }
}