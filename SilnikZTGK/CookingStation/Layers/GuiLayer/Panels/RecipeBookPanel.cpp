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
    m_SandwichIcon = AssetManager::GetTexture("assets://UI/sandwich.png");
    m_FriedEggIcon = AssetManager::GetTexture("assets://UI/friedEgg.png"); // Podmieñ jeœli masz inn¹ nazwê!
    m_EggsAndBaconIcon = AssetManager::GetTexture("assets://UI/eggsAndBacon.png");
    m_ShakshukaIcon = AssetManager::GetTexture("assets://UI/shakshuka.png");
    m_BaguetteIcon = AssetManager::GetTexture("assets://UI/baguette.png");

    // Obrazki ca³ych przepisów (chmurki)
    m_TomatoSoupRecipeTex = AssetManager::GetTexture("assets://UI/TomatoSoupRecipe.png");
    m_SandwichRecipeTex = AssetManager::GetTexture("assets://UI/SandwichRecipe.png");
    m_FriedEggRecipeTex = AssetManager::GetTexture("assets://UI/FriedEggRecipe.png");
    m_EggsAndBaconRecipeTex = AssetManager::GetTexture("assets://UI/EggsAndBaconRecipe.png");
    m_ShakshukaRecipeTex = AssetManager::GetTexture("assets://UI/ShakshukaRecipe.png");
    m_BaguetteRecipeTex = AssetManager::GetTexture("assets://UI/BaguetteRecipe.png");
}


void RecipeBookPanel::DrawRecipeIcon(const std::string& recipeId, const std::string& displayName, const std::shared_ptr<Texture>& iconTex, const std::shared_ptr<Texture>& tooltipTex,
    glm::vec2 relativePct, float targetWidth, glm::vec2 bookPos, glm::vec2 bookSize, float dt, bool isBlocked,
    std::shared_ptr<Texture>& outTooltipTex, glm::vec2& outTooltipPos, glm::vec2& outTooltipSize)
{
    if (!iconTex) return;

    // MAGIA 1: Szerokoœæ jest sta³a, a wysokoœæ dopasowuje siê proporcjonalnie!
    float aspect = (float)iconTex->GetWidth() / (float)iconTex->GetHeight();
    glm::vec2 size = { targetWidth, targetWidth / aspect };

    // MAGIA 2: Pozycja z procentów to teraz ŒRODEK ikonki, a nie jej róg.
    glm::vec2 slotCenter = { bookPos.x + bookSize.x * relativePct.x, bookPos.y + bookSize.y * relativePct.y };
    glm::vec2 pos = { slotCenter.x - size.x * 0.5f, slotCenter.y - size.y * 0.5f };

    bool isUnlocked = GameProgress::IsRecipeUnlocked(recipeId);

    glm::vec4 tint = isUnlocked ? glm::vec4(1.0f) : glm::vec4(0.0f, 0.0f, 0.0f, 0.8f);
    BubblyUI::DrawBubblyImage(m_BubblyStates, "Recipe_" + recipeId, iconTex, pos, size, dt, isBlocked, 1.15f, true, 0.5f, tint);

    // --- PODPIS POD IKONK¥ ---
    float baseScale = targetWidth / 120.0f;
    float textScale = 0.70f * baseScale;
    std::string textToShow = isUnlocked ? displayName : "???";
    float textW = Gui::MeasureTextWidth(textToShow, textScale);

    // Tekst wyœrodkowany pod ikonk¹
    glm::vec2 textPos = { slotCenter.x - textW * 0.5f, pos.y + size.y + (8.0f * baseScale) };

    glm::vec4 purpleColor = { 0.75f, 0.4f, 0.9f, 1.0f };
    Gui::DrawGuiText(textToShow, { textPos.x + 1.5f, textPos.y + 1.5f }, textScale, { 0.0f, 0.0f, 0.0f, 0.2f });
    Gui::DrawGuiText(textToShow, textPos, textScale, purpleColor);

    // --- OBS£UGA CHMURKI ---
    glm::vec2 mouse = Gui::GetMappedMousePos();
    bool isHovered = (mouse.x >= pos.x && mouse.x <= pos.x + size.x && mouse.y >= pos.y && mouse.y <= pos.y + size.y);

    if (isHovered && tooltipTex && !isBlocked) {
        float tooltipHeight = targetWidth * 2.3f;
        glm::vec2 tSize = GuiUtils::CalculateAspectSize(tooltipTex, tooltipHeight);

        float margin = 10.0f * baseScale;
        glm::vec2 tPos;

        // Zgodnie z ¿yczeniem: Lewa kolumna -> chmurka w lewo. Prawa kolumna -> chmurka w prawo.
        if (relativePct.x < 0.25f) {
            tPos.x = pos.x - tSize.x - margin;
        }
        else {
            tPos.x = pos.x + size.x + margin;
        }

        // Chmurka wyœrodkowana w pionie ze œrodkiem ikonki
        tPos.y = slotCenter.y - tSize.y * 0.5f;

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

        // --- RYSOWANIE IKONEK (Siatka 2 kolumny x 3 wiersze) ---
        float recipeW = 100.0f * baseScale; 
        
        std::shared_ptr<Texture> activeTooltipTex = nullptr;
        glm::vec2 activeTooltipPos;
        glm::vec2 activeTooltipSize;

        // Rozsuniête kolumny i wiersze na lewej stronie ksi¹¿ki
        float col1 = 0.18f;  // Mocniej do lewej krawêdzi kartki
        float col2 = 0.38f;  // Bli¿ej zgiêcia na œrodku ksi¹¿ki
        
        float row1 = 0.20f;  // Troszkê wy¿ej
        float row2 = 0.40f;  // Idealny œrodek
        float row3 = 0.62f;  // Troszkê ni¿ej

        // --- RZ¥D 1 ---

        // --- RZ¥D 1 ---
        DrawRecipeIcon("TomatoSoup", "Tomato Soup", m_TomatoSoupIcon, m_TomatoSoupRecipeTex,
            { col1, row1 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize);
        DrawRecipeIcon("FriedEggs", "Fried Eggs", m_FriedEggIcon, m_FriedEggRecipeTex,
            { col2, row1 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize);

        // --- RZ¥D 2 ---
        DrawRecipeIcon("EggsAndBacon", "Eggs & Bacon", m_EggsAndBaconIcon, m_EggsAndBaconRecipeTex,
            { col1, row2 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize);
        DrawRecipeIcon("Shakshuka", "Shakshuka", m_ShakshukaIcon, m_ShakshukaRecipeTex,
            { col2, row2 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize);

        // --- RZ¥D 3 ---
        DrawRecipeIcon("Baguette", "Baguette", m_BaguetteIcon, m_BaguetteRecipeTex,
            { col1, row3 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize);
        DrawRecipeIcon("Sandwich", "Sandwich", m_SandwichIcon, m_SandwichRecipeTex,
            { col2, row3 }, recipeW, insidePos, insideSize, dt, isBlocked, activeTooltipTex, activeTooltipPos, activeTooltipSize);

        // --- RYSOWANIE CHMURKI NA SAMYM WIERZCHU ---
        if (activeTooltipTex) {
            Renderer2D::DrawQuad(activeTooltipPos, activeTooltipSize, activeTooltipTex, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
        }
    }
}