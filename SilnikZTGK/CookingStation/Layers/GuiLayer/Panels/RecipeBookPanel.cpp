#include "RecipeBookPanel.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Layers/GuiLayer/Utils/GuiUtils.h"
#include "CookingStation/Core/GameProgress.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Layers/GuiLayer/Utils/AudioConfig.h"
#include "CookingStation/Layers/GuiLayer/Utils/Gui.h" 

void RecipeBookPanel::Init() {
    m_BookCloudIcon = AssetManager::GetTexture("assets://UI/bookCloud.png");
    m_BookIcon = AssetManager::GetTexture("assets://UI/book.png");
    m_BookStarsIcon = AssetManager::GetTexture("assets://UI/bookStars.png");
    m_BookInsideIcon = AssetManager::GetTexture("assets://UI/bookInside.png");
    m_BookXIcon = AssetManager::GetTexture("assets://UI/bookX.png");

    // Ikonki dañ
    m_TomatoSoupIcon = AssetManager::GetTexture("assets://UI/tomatoSoup.png");

    // Obrazki ca³ych przepisów (chmurki)
    m_TomatoSoupRecipeTex = AssetManager::GetTexture("assets://UI/TomatoSoupRecipe.png");
}


void RecipeBookPanel::DrawRecipeIcon(const std::string& recipeId, const std::string& displayName, const std::shared_ptr<Texture>& iconTex, const std::shared_ptr<Texture>& tooltipTex,
    glm::vec2 relativePct, float targetHeight, glm::vec2 bookPos, glm::vec2 bookSize, float dt, bool isBlocked,
    std::shared_ptr<Texture>& outTooltipTex, glm::vec2& outTooltipPos, glm::vec2& outTooltipSize)
{
    if (!iconTex) return;

    glm::vec2 size = GuiUtils::CalculateAspectSize(iconTex, targetHeight);
    glm::vec2 pos = { bookPos.x + bookSize.x * relativePct.x, bookPos.y + bookSize.y * relativePct.y };

    bool isUnlocked = GameProgress::IsRecipeUnlocked(recipeId);

    // Zablokowane = czarna sylwetka, Odblokowane = pe³ne kolory
    glm::vec4 tint = isUnlocked ? glm::vec4(1.0f) : glm::vec4(0.0f, 0.0f, 0.0f, 0.8f);
    BubblyUI::DrawBubblyImage(m_BubblyStates, "Recipe_" + recipeId, iconTex, pos, size, dt, isBlocked, 1.15f, true, 0.5f, tint);

    // --- PODPIS POD IKONK¥ (Fioletowy z tutoriala) ---
    // Pobieramy bazow¹ skalê na podstawie wysokoœci ikonki
    float baseScale = targetHeight / 120.0f;
    float textScale = 0.75f * baseScale;
    std::string textToShow = isUnlocked ? displayName : "???";
    float textW = Gui::MeasureTextWidth(textToShow, textScale);
    glm::vec2 textPos = { pos.x + (size.x - textW) * 0.5f, pos.y + size.y + (10.0f * baseScale) };

    glm::vec4 purpleColor = { 0.75f, 0.4f, 0.9f, 1.0f }; // Kolor Waltera!
    Gui::DrawGuiText(textToShow, { textPos.x + 2.0f, textPos.y + 2.0f }, textScale, { 0.0f, 0.0f, 0.0f, 0.5f }); // Cieñ
    Gui::DrawGuiText(textToShow, textPos, textScale, purpleColor);

    // --- OBS£UGA DU¯EJ CHMURKI (HOVER) ---
    glm::vec2 mouse = Gui::GetMappedMousePos();
    bool isHovered = (mouse.x >= pos.x && mouse.x <= pos.x + size.x && mouse.y >= pos.y && mouse.y <= pos.y + size.y);

    if (isHovered && tooltipTex && !isBlocked) {
        // Chmurka znacznie wiêksza (2.3x wysokoœæ ikonki)
        float tooltipHeight = targetHeight * 2.3f;
        glm::vec2 tSize = GuiUtils::CalculateAspectSize(tooltipTex, tooltipHeight);

        glm::vec2 tPos;
        float margin = 20.0f * baseScale;

        // Jeœli ikonka jest po lewej stronie ksi¹¿ki (< 0.5), wyrzuæ chmurkê na PRAWO (do œrodka), i na odwrót
        if (relativePct.x < 0.5f) {
            tPos.x = pos.x + size.x + margin;
        }
        else {
            tPos.x = pos.x - tSize.x - margin;
        }

        // Wyœrodkowanie chmurki w pionie wzglêdem ikonki
        tPos.y = pos.y + (size.y - tSize.y) * 0.5f;

        outTooltipTex = tooltipTex;
        outTooltipPos = tPos;
        outTooltipSize = tSize;
    }
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

        if (BubblyUI::DrawBubblyImage(m_BubblyStates, "BookIcon", m_BookIcon, bookPos, bookSize, dt, isBlocked, 1.15f, true, 0.35f)) {
            m_IsOpen = true;
            AudioEngine::Play(AudioConfig::BookOpenSound);
        }
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
            if (BubblyUI::DrawBubblyImage(m_BubblyStates, "BookX", m_BookXIcon, xPos, xSize, dt, false, 1.2f, true, 0.4f)) {
                m_IsOpen = false;
                AudioEngine::Play(AudioConfig::BookCloseSound);
            }
        }

        // --- RYSOWANIE IKONEK (Z Tooltipami) ---
        float recipeH = 120.0f * baseScale;

        // Zmienne, które "przechwyc¹" chmurkê, jeœli jakaœ jest najechana myszk¹
        std::shared_ptr<Texture> activeTooltipTex = nullptr;
        glm::vec2 activeTooltipPos;
        glm::vec2 activeTooltipSize;

        // Ikonka pomidorówki na lewej stronie ksi¹¿ki
        DrawRecipeIcon(
            "TomatoSoup", "Tomato Soup", m_TomatoSoupIcon, m_TomatoSoupRecipeTex,
            { 0.15f, 0.15f }, recipeH, insidePos, insideSize, dt, isBlocked,
            activeTooltipTex, activeTooltipPos, activeTooltipSize
        );

        // --- RYSOWANIE CHMURKI NA SAMYM WIERZCHU ---
        // Rysujemy to dopiero tutaj, ¿eby inne ikonki nie przykry³y nam naszego przepisu!
        if (activeTooltipTex) {
            Renderer2D::DrawQuad(activeTooltipPos, activeTooltipSize, activeTooltipTex, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
        }
    }
}