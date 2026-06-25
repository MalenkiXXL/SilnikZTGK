#include "RecipeBookPanel.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Layers/GuiLayer/Utils/GuiUtils.h"
#include "CookingStation/Core/GameProgress.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Layers/GuiLayer/Utils/AudioConfig.h"
#include "CookingStation/Layers/GuiLayer/Utils/Gui.h" 
#include "CookingStation/Core/Input.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <algorithm>
#include "CookingStation/Scripts/Managers/GameManagerScript.h"

void RecipeBookPanel::Init() {
    m_BookCloudIcon = AssetManager::GetTexture("assets://UI/bookCloud.png");
    m_BookIcon = AssetManager::GetTexture("assets://UI/book.png");
    m_BookStarsIcon = AssetManager::GetTexture("assets://UI/bookStars.png");
    m_BookInsideIcon = AssetManager::GetTexture("assets://UI/bookInside.png");
    m_BookXIcon = AssetManager::GetTexture("assets://UI/bookX.png");

    m_LockClosedIcon = AssetManager::GetTexture("assets://UI/lockClosed.png");
    m_NextPageRightIcon = AssetManager::GetTexture("assets://UI/nextPageRight.png");
    m_NextPageLeftIcon = AssetManager::GetTexture("assets://UI/nextPageLeft.png");

    m_TomatoSoupIcon = AssetManager::GetTexture("assets://UI/tomatoSoup.png");
    m_SandwichIcon = AssetManager::GetTexture("assets://UI/sandwich.png");
    m_FriedEggIcon = AssetManager::GetTexture("assets://UI/friedEgg.png");
    m_EggsAndBaconIcon = AssetManager::GetTexture("assets://UI/eggsAndBacon.png");
    m_ShakshukaIcon = AssetManager::GetTexture("assets://UI/shakshuka.png");
    m_BaguetteIcon = AssetManager::GetTexture("assets://UI/baguette.png");

    m_CapreseIcon = AssetManager::GetTexture("assets://UI/caprese.png");
    m_ShakeIcon = AssetManager::GetTexture("assets://UI/shake.png");
    m_ApplePieIcon = AssetManager::GetTexture("assets://UI/applePie.png");
    m_KopytkaIcon = AssetManager::GetTexture("assets://UI/kopytka.png");
    m_CupcakeIcon = AssetManager::GetTexture("assets://UI/cupcake.png");
    m_CroissantIcon = AssetManager::GetTexture("assets://UI/croissant.png");

    m_CoffeeIcon = AssetManager::GetTexture("assets://UI/coffeeCup.png");
    m_CandyIcon = AssetManager::GetTexture("assets://UI/candy.png");

    m_TomatoSoupRecipeTex = AssetManager::GetTexture("assets://UI/TomatoSoupRecipe.png");
    m_SandwichRecipeTex = AssetManager::GetTexture("assets://UI/SandwichRecipe.png");
    m_FriedEggRecipeTex = AssetManager::GetTexture("assets://UI/FriedEggRecipe.png");
    m_EggsAndBaconRecipeTex = AssetManager::GetTexture("assets://UI/EggsAndBaconRecipe.png");
    m_ShakshukaRecipeTex = AssetManager::GetTexture("assets://UI/ShakshukaRecipe.png");
    m_BaguetteRecipeTex = AssetManager::GetTexture("assets://UI/BaguetteRecipe.png");

    m_CapreseRecipeTex = AssetManager::GetTexture("assets://UI/capreseRecipe.png");
    m_ShakeRecipeTex = AssetManager::GetTexture("assets://UI/ShakeRecipe.png");
    m_ApplePieRecipeTex = AssetManager::GetTexture("assets://UI/ApplePieRecipe.png");
    m_KopytkaRecipeTex = AssetManager::GetTexture("assets://UI/KopytkaRecipe.png");
    m_CupcakeRecipeTex = AssetManager::GetTexture("assets://UI/CupcakeRecipe.png");
    m_CroissantRecipeTex = AssetManager::GetTexture("assets://UI/CroissantRecipe.png");

    m_CoffeeRecipeTex = AssetManager::GetTexture("assets://UI/CoffeeRecipe.png");
    m_CandyRecipeTex = AssetManager::GetTexture("assets://UI/CandyRecipe.png");
}

void RecipeBookPanel::DrawRecipeIcon(const std::string& recipeId, const std::string& displayName, const std::shared_ptr<Texture>& iconTex, const std::shared_ptr<Texture>& tooltipTex,
    glm::vec2 relativePct, float targetWidth, glm::vec2 bookPos, glm::vec2 bookSize, float dt, bool isBlocked,
    std::shared_ptr<Texture>& outTooltipTex, glm::vec2& outTooltipPos, glm::vec2& outTooltipSize, bool isGrandmaLocked, float customScale, glm::vec2 customIconOffset)
{
    if (!iconTex) return;

    float aspect = (float)iconTex->GetWidth() / (float)iconTex->GetHeight();
    glm::vec2 size;
    if (aspect >= 1.0f) {
        size = { targetWidth, targetWidth / aspect };
    }
    else {
        size = { targetWidth * aspect, targetWidth };
    }

    size *= customScale;

    glm::vec2 slotCenter = { bookPos.x + bookSize.x * relativePct.x, bookPos.y + bookSize.y * relativePct.y };

    glm::vec2 pos = { slotCenter.x - size.x * 0.5f + customIconOffset.x, slotCenter.y - size.y * 0.5f + customIconOffset.y };

    bool isUnlocked = GameProgress::IsRecipeUnlocked(recipeId);

    glm::vec4 tint;
    if (isGrandmaLocked) {
        isUnlocked = false;
        tint = glm::vec4(0.12f, 0.12f, 0.12f, 1.0f);
    }
    else if (isUnlocked) {
        tint = glm::vec4(1.0f);
    }
    else {
        tint = glm::vec4(0.0f, 0.0f, 0.0f, 0.8f);
    }

    BubblyUI::DrawBubblyImage(m_BubblyStates, "Recipe_" + recipeId, iconTex, pos, size, dt, isBlocked, 1.15f, true, 0.5f, tint);

    // K��dka (zostaje na �rodku slota)
    if (isGrandmaLocked && m_LockClosedIcon) {
        float lockSize = targetWidth * 0.45f;
        float lockAspect = (float)m_LockClosedIcon->GetWidth() / (float)m_LockClosedIcon->GetHeight();
        glm::vec2 lockDims = { lockSize * lockAspect, lockSize };
        glm::vec2 lockPos = { slotCenter.x - lockDims.x * 0.5f, slotCenter.y - lockDims.y * 0.5f };

        Renderer2D::DrawQuad(lockPos, lockDims, m_LockClosedIcon, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    }

    float baseScale = targetWidth / 120.0f;
    float textScale = 0.70f * baseScale;
    std::string textToShow = isUnlocked ? displayName : "???";
    float textW = Gui::MeasureTextWidth(textToShow, textScale);

    glm::vec2 textPos = { slotCenter.x - textW * 0.5f, slotCenter.y + (size.y / customScale) * 0.5f + (8.0f * baseScale) };

    glm::vec4 purpleColor = { 0.75f, 0.4f, 0.9f, 1.0f };
    Gui::DrawGuiText(textToShow, { textPos.x + 1.5f, textPos.y + 1.5f }, textScale, { 0.0f, 0.0f, 0.0f, 0.2f });
    Gui::DrawGuiText(textToShow, textPos, textScale, purpleColor);

    glm::vec2 mouse = Gui::GetMappedMousePos();
    bool isHovered = (mouse.x >= pos.x && mouse.x <= pos.x + size.x && mouse.y >= pos.y && mouse.y <= pos.y + size.y);

    if (isHovered && tooltipTex && !isBlocked && !isGrandmaLocked) {
        float tooltipHeight = targetWidth * 2.3f;
        glm::vec2 tSize = GuiUtils::CalculateAspectSize(tooltipTex, tooltipHeight);

        float margin = 10.0f * baseScale;
        glm::vec2 tPos;

        if (relativePct.x < 0.25f || (relativePct.x > 0.5f && relativePct.x < 0.75f)) {
            tPos.x = pos.x - tSize.x - margin;
        }
        else {
            tPos.x = pos.x + size.x + margin;
        }

        tPos.y = slotCenter.y - tSize.y * 0.5f;

        outTooltipTex = tooltipTex;
        outTooltipPos = tPos;
        outTooltipSize = tSize;
    }
}


void RecipeBookPanel::Draw(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt, bool isGamePaused) {
    if (!m_BookIcon) return;

    bool isBlocked = isGamePaused;
    
    bool isGrandmaUnlocked = false;
    if (GameManagerScript::s_Instance &&
        (GameManagerScript::s_Instance->m_IsMapExpanding || GameManagerScript::s_Instance->m_MapExpandProgress > 0.0f)) {
        isGrandmaUnlocked = true;
    }

    int currentUnlocked = 0;
    for (const auto& pair : GameProgress::UnlockedRecipes) {
        if (pair.second) currentUnlocked++;
    }

    if (currentUnlocked > m_LastUnlockedCount) {
        m_GlowTimer = 2.5f;
        m_LastUnlockedCount = currentUnlocked;
        m_HasShownLostHint = true;
    }

    if (!m_HasShownLostHint && currentUnlocked == 0) {
        m_GameTime += dt;
        if (m_GameTime >= 60.0f) {
            m_HasShownLostHint = true;
            m_LostHintTimer = 8.0f;
            m_GlowTimer = 8.0f;
        }
    }

    if (m_GlowTimer > 0.0f) m_GlowTimer -= dt;
    if (m_LostHintTimer > 0.0f) m_LostHintTimer -= dt;

    glm::vec2 cloudSize = { 210.0f * baseScale, 210.0f * baseScale };
    glm::vec2 cloudPos = { gameX + 10.0f * baseScale, gameY * baseScale };
    glm::vec2 actualCloudSize = cloudSize * 1.3f;

    if (m_BookCloudIcon) BubblyUI::DrawBubblyImage(m_BubblyStates, "BookCloud", m_BookCloudIcon, cloudPos, actualCloudSize, dt, false, 1.1f, false);

    if (!m_IsOpen) {
        glm::vec2 bookSize = cloudSize * 1.1f;
        glm::vec2 bookPos = { cloudPos.x + (actualCloudSize.x - bookSize.x) * 0.5f, cloudPos.y + (actualCloudSize.y - bookSize.y) * 0.5f };

        bool bookClicked = BubblyUI::DrawBubblyImage(m_BubblyStates, "BookIcon", m_BookIcon, bookPos, bookSize, dt, isBlocked, 1.15f, true, 0.35f);

        if (m_GlowTimer > 0.0f) {
            float timeNow = glfwGetTime();
            float wave = (std::sin(timeNow * 6.0f) + 1.0f) * 0.5f;
            float flashSpike = std::pow(wave, 4.0f);

            float glowScale = 1.0f + (flashSpike * 0.25f);
            glm::vec2 glowSize = bookSize * glowScale;
            glm::vec2 glowPos = {
                bookPos.x - (glowSize.x - bookSize.x) * 0.5f,
                bookPos.y - (glowSize.y - bookSize.y) * 0.5f
            };

            glm::vec4 flashColor = { 1.0f, 0.65f, 0.95f, flashSpike * 0.95f };
            Renderer2D::DrawQuad(glowPos, glowSize, m_BookIcon, flashColor, { 0.0f, 1.0f }, { 1.0f, 0.0f });
        }

        if (bookClicked) {
            m_IsOpen = true;
            m_CurrentPage = 0;
            AudioEngine::Play(AudioConfig::BookOpenSound);
            m_GlowTimer = 0.0f;
            m_LostHintTimer = 0.0f;
        }

        if (m_BookStarsIcon) BubblyUI::DrawBubblyImage(m_BubblyStates, "BookStars", m_BookStarsIcon, cloudPos, actualCloudSize, dt, isBlocked, 1.15f, false);

        if (m_LostHintTimer > 0.0f) {
            std::string line1 = "Lost?";
            std::string line2 = "Check the recipes here";

            float hintScale = 1.0f * baseScale;
            float textAlpha = 1.0f;
            if (m_LostHintTimer > 7.5f) { textAlpha = (8.0f - m_LostHintTimer) / 0.5f; }
            else if (m_LostHintTimer < 0.5f) { textAlpha = m_LostHintTimer / 0.5f; }
            textAlpha = std::clamp(textAlpha, 0.0f, 1.0f);

            float timeNow = glfwGetTime();
            float floatOffset = std::sin(timeNow * 2.2f) * 5.0f * baseScale;

            float w1 = Gui::MeasureTextWidth(line1, hintScale);
            float w2 = Gui::MeasureTextWidth(line2, hintScale);
            float maxW = std::max(w1, w2);

            float textH = Gui::MeasureTextHeight("A", hintScale);
            float lineSpacing = textH * 1.3f;
            float totalH = textH + lineSpacing;

            glm::vec2 blockPos = { bookPos.x + bookSize.x + 20.0f * baseScale, bookPos.y + (bookSize.y - totalH) * 0.5f + floatOffset };
            float line1X = blockPos.x + (maxW - w1) * 0.5f;
            float line2X = blockPos.x + (maxW - w2) * 0.5f;

            glm::vec4 shadowColor = { 0.0f, 0.0f, 0.0f, 0.4f * textAlpha };
            glm::vec4 textColor = { 1.0f, 1.0f, 1.0f, 1.0f * textAlpha };

            Gui::DrawGuiText(line1, { line1X + 1.5f, blockPos.y + 1.5f }, hintScale, shadowColor);
            Gui::DrawGuiText(line1, { line1X, blockPos.y }, hintScale, textColor);
            Gui::DrawGuiText(line2, { line2X + 1.5f, blockPos.y + lineSpacing + 1.5f }, hintScale, shadowColor);
            Gui::DrawGuiText(line2, { line2X, blockPos.y + lineSpacing }, hintScale, textColor);
        }
    }
    else {
        glm::vec2 insideSize = GuiUtils::CalculateAspectSize(m_BookInsideIcon, gameHeight * 1.0f);
        float yOffset = 50.0f * baseScale;
        glm::vec2 insidePos = { gameX + (gameWidth - insideSize.x) * 0.5f, gameY + (gameHeight - insideSize.y) * 0.5f + yOffset };

        if (m_BookInsideIcon) BubblyUI::DrawBubblyImage(m_BubblyStates, "BookInside", m_BookInsideIcon, insidePos, insideSize, dt, false, 1.0f, false);

        glm::vec2 xSize = { 50.0f * baseScale, 50.0f * baseScale };
        glm::vec2 xPos = { insidePos.x + insideSize.x - xSize.x * 2.5f, insidePos.y + xSize.y * 2.9f };

        bool closeClicked = false;
        if (m_BookXIcon) {
            closeClicked = BubblyUI::DrawBubblyImage(m_BubblyStates, "BookX", m_BookXIcon, xPos, xSize, dt, false, 1.2f, true, 0.4f);
        }

        if (closeClicked || Input::IsKeyPressed(GLFW_KEY_ESCAPE)) {
            m_IsOpen = false;
            AudioEngine::Play(AudioConfig::BookCloseSound);
        }

        if (m_NextPageLeftIcon && m_NextPageRightIcon) {
            float arrowH = 40.0f * baseScale;
            glm::vec2 arrowLeftSize = GuiUtils::CalculateAspectSize(m_NextPageLeftIcon, arrowH);
            glm::vec2 arrowRightSize = GuiUtils::CalculateAspectSize(m_NextPageRightIcon, arrowH);

            glm::vec2 leftBtnPos = {
                insidePos.x + insideSize.x * 0.08f,
                insidePos.y + insideSize.y * 0.71f - arrowLeftSize.y * 0.5f
            };

            glm::vec2 rightBtnPos = {
                insidePos.x + insideSize.x * 0.93f - arrowRightSize.x,
                insidePos.y + insideSize.y * 0.71f - arrowRightSize.y * 0.5f
            };

            if (m_CurrentPage > 0) {
                if (BubblyUI::DrawBubblyImage(m_BubblyStates, "PrevPageBtn", m_NextPageLeftIcon, leftBtnPos, arrowLeftSize, dt, isBlocked, 1.15f, true)) {
                    m_CurrentPage--;
                    AudioEngine::Play(AudioConfig::BookOpenSound);
                }
            }
            if (m_CurrentPage < m_MaxPages) {
                if (BubblyUI::DrawBubblyImage(m_BubblyStates, "NextPageBtn", m_NextPageRightIcon, rightBtnPos, arrowRightSize, dt, isBlocked, 1.15f, true)) {
                    m_CurrentPage++;
                    AudioEngine::Play(AudioConfig::BookOpenSound);
                }
            }
        }

        float recipeW = 100.0f * baseScale;
        std::shared_ptr<Texture> activeTooltipTex = nullptr;
        glm::vec2 activeTooltipPos;
        glm::vec2 activeTooltipSize;

        float col1 = 0.18f; float col2 = 0.38f;
        float col3 = 0.62f; float col4 = 0.82f;
        float row1 = 0.20f; float row2 = 0.40f; float row3 = 0.62f;

        if (m_CurrentPage == 0) {
            DrawRecipeIcon("TomatoSoup", "Tomato Soup", m_TomatoSoupIcon, m_TomatoSoupRecipeTex,
                { col1, row1 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize, false);
            DrawRecipeIcon("FriedEggs", "Fried Eggs", m_FriedEggIcon, m_FriedEggRecipeTex,
                { col2, row1 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize, false);
            DrawRecipeIcon("EggsAndBacon", "Eggs & Bacon", m_EggsAndBaconIcon, m_EggsAndBaconRecipeTex,
                { col1, row2 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize, false);
            DrawRecipeIcon("Shakshuka", "Shakshuka", m_ShakshukaIcon, m_ShakshukaRecipeTex,
                { col2, row2 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize, false);
            DrawRecipeIcon("Baguette", "Baguette", m_BaguetteIcon, m_BaguetteRecipeTex,
                { col1, row3 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize, false);
            DrawRecipeIcon("Sandwich", "Sandwich", m_SandwichIcon, m_SandwichRecipeTex,
                { col2, row3 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize, false);

            DrawRecipeIcon("Caprese", "Caprese", m_CapreseIcon, m_CapreseRecipeTex,
                { col3, row1 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize, false);
            DrawRecipeIcon("Shake", "Shake", m_ShakeIcon, m_ShakeRecipeTex,
                { col4, row1 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize, !isGrandmaUnlocked);
            DrawRecipeIcon("ApplePie", "Apple Pie", m_ApplePieIcon, m_ApplePieRecipeTex,
                { col3, row2 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize, !isGrandmaUnlocked);
            DrawRecipeIcon("Kopytka", "Kopytka", m_KopytkaIcon, m_KopytkaRecipeTex,
                { col4, row2 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize, !isGrandmaUnlocked);
            DrawRecipeIcon("Cupcake", "Cupcake", m_CupcakeIcon, m_CupcakeRecipeTex,
                { col3, row3 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize, !isGrandmaUnlocked);
            DrawRecipeIcon("SleepyBread", "Croissant", m_CroissantIcon, m_CroissantRecipeTex,
                { col4, row3 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize, !isGrandmaUnlocked);
        }
        else if (m_CurrentPage == 1) {
            // ZMIANA: Przesuwamy kaw� w PRAWO (dodatnia warto�� X offsetu)
            DrawRecipeIcon("Coffee", "Coffee", m_CoffeeIcon, m_CoffeeRecipeTex,
                { col1, row1 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize, !isGrandmaUnlocked, 0.8f, { 8.0f * baseScale, 0.0f });

            DrawRecipeIcon("Candy", "Candy", m_CandyIcon, m_CandyRecipeTex,
                { col2, row1 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize, !isGrandmaUnlocked, 1.15f);
        }

        if (activeTooltipTex) {
            Renderer2D::DrawQuad(activeTooltipPos, activeTooltipSize, activeTooltipTex, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
        }
    }
}