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
    void Reset() {
        m_HasShownLostHint = false;
        m_GameTime = 0.0f;
        m_GlowTimer = 0.0f;
        m_LostHintTimer = 0.0f;
    }

private:
    // ZMIANA: Dodano customScale i customIconOffset do rêcznego poprawiania niesfornych ikon!
    void DrawRecipeIcon(const std::string& recipeId, const std::string& displayName, const std::shared_ptr<Texture>& iconTex, const std::shared_ptr<Texture>& tooltipTex,
        glm::vec2 relativePct, float targetWidth, glm::vec2 bookPos, glm::vec2 bookSize, float dt, bool isBlocked,
        std::shared_ptr<Texture>& outTooltipTex, glm::vec2& outTooltipPos, glm::vec2& outTooltipSize,
        bool isGrandmaLocked = false, float customScale = 1.0f, glm::vec2 customIconOffset = { 0.0f, 0.0f });

    bool m_IsOpen = false;
    std::unordered_map<std::string, BubblyState> m_BubblyStates;

    std::shared_ptr<Texture> m_BookCloudIcon;
    std::shared_ptr<Texture> m_BookIcon;
    std::shared_ptr<Texture> m_BookStarsIcon;
    std::shared_ptr<Texture> m_BookInsideIcon;
    std::shared_ptr<Texture> m_BookXIcon;

    // --- IKONKA K£ÓDKI I PRZEWIJANIA STRON ---
    std::shared_ptr<Texture> m_LockClosedIcon;
    std::shared_ptr<Texture> m_NextPageLeftIcon;
    std::shared_ptr<Texture> m_NextPageRightIcon;

    // --- STRONICOWANIE ---
    int m_CurrentPage = 0;
    int m_MaxPages = 1;

    // --- IKONKI DAÑ (Strona 1 - lewa) ---
    std::shared_ptr<Texture> m_TomatoSoupIcon;
    std::shared_ptr<Texture> m_SandwichIcon;
    std::shared_ptr<Texture> m_FriedEggIcon;
    std::shared_ptr<Texture> m_EggsAndBaconIcon;
    std::shared_ptr<Texture> m_ShakshukaIcon;
    std::shared_ptr<Texture> m_BaguetteIcon;

    // --- IKONKI DAÑ (Strona 1 - prawa) ---
    std::shared_ptr<Texture> m_CapreseIcon;
    std::shared_ptr<Texture> m_ShakeIcon;
    std::shared_ptr<Texture> m_ApplePieIcon;
    std::shared_ptr<Texture> m_KopytkaIcon;
    std::shared_ptr<Texture> m_CupcakeIcon;
    std::shared_ptr<Texture> m_CroissantIcon;

    // --- IKONKI DAÑ (Strona 2) ---
    std::shared_ptr<Texture> m_CoffeeIcon;
    std::shared_ptr<Texture> m_CandyIcon;

    // --- TEKSTURY PRZEPISÓW (Strona 1 - lewa) ---
    std::shared_ptr<Texture> m_TomatoSoupRecipeTex;
    std::shared_ptr<Texture> m_SandwichRecipeTex;
    std::shared_ptr<Texture> m_FriedEggRecipeTex;
    std::shared_ptr<Texture> m_EggsAndBaconRecipeTex;
    std::shared_ptr<Texture> m_ShakshukaRecipeTex;
    std::shared_ptr<Texture> m_BaguetteRecipeTex;

    // --- TEKSTURY PRZEPISÓW (Strona 1 - prawa) ---
    std::shared_ptr<Texture> m_CapreseRecipeTex;
    std::shared_ptr<Texture> m_ShakeRecipeTex;
    std::shared_ptr<Texture> m_ApplePieRecipeTex;
    std::shared_ptr<Texture> m_KopytkaRecipeTex;
    std::shared_ptr<Texture> m_CupcakeRecipeTex;
    std::shared_ptr<Texture> m_CroissantRecipeTex;

    // --- TEKSTURY PRZEPISÓW (Strona 2) ---
    std::shared_ptr<Texture> m_CoffeeRecipeTex;
    std::shared_ptr<Texture> m_CandyRecipeTex;

    int m_LastUnlockedCount = 0;
    float m_GlowTimer = 0.0f;
    float m_GameTime = 0.0f;
    bool m_HasShownLostHint = false;
    float m_LostHintTimer = 0.0f;
};