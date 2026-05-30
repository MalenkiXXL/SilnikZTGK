#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Events/MouseEvent.h"
#include "CookingStation/Core/Timestep.h"
#include "CookingStation/Scene/Scene.h"
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "CookingStation/Core/Texture.h"
#include <memory>
#include "CookingStation/Renderer/Framebuffer.h"
#include "CarouselUI.h"
#include <unordered_map>

class Scene;

class GameGuiLayer : public Layer {
public:
    GameGuiLayer() : Layer("GameGuiLayer") {}
    virtual ~GameGuiLayer() = default;

    void SetVisible(bool visible) { m_IsVisible = visible; }
    bool IsVisible() const { return m_IsVisible; }

    virtual void OnAttach() override;
    virtual void OnUpdate(Timestep ts) override;
    virtual void OnEvent(Event& e) override;
    virtual void OnDetach() override;

    void SetViewportFramebuffer(const std::shared_ptr<Framebuffer>& fbo) { m_ViewportFBO = fbo; }
    void ReloadQuests();

    static bool s_NeedsQuestReload;

private:
    // --- Widocznoœæ / pauza ---
    bool m_IsVisible = false;
    bool m_IsPaused = false;
    bool m_IsActive = false;

    // --- Skale przycisków menu pauzy ---
    float m_ReturnBtnScale = 1.0f;
    float m_SettingsBtnScale = 1.0f;
    float m_ExitBtnScale = 1.0f;

    bool m_SettingsOpen = false;
    int m_PendingResIndex = 0;
    int m_PendingMsaaIndex = 0;
    std::vector<int> m_MsaaOptions = { 1, 2, 4, 8 };

    float m_BackBtnScale = 1.0f;
    float m_ApplyBtnScale = 1.0f;
    float m_ResLeftBtnScale = 1.0f;
    float m_ResRightBtnScale = 1.0f;
    float m_MsaaLeftBtnScale = 1.0f;
    float m_MsaaRightBtnScale = 1.0f;

    static constexpr int MsaaOptions[] = { 1, 2, 4, 8 };
    static constexpr int MsaaOptionCount = 4;

    // -----------------------------------------------------------------------
    // EventBus – ID subskrypcji (potrzebne do bezpiecznego Unsubscribe)
    // -----------------------------------------------------------------------
    std::size_t m_InventorySubId = 0;   // scena-level bus (InventoryChangedEvent)
    std::size_t m_MoneySubId = 0;   // scena-level bus (MoneyChangedEvent)
    std::size_t m_GameStartedSubId = 0;  // application-level bus (GameStartedEvent)

    // --- Scena ---
    std::shared_ptr<Scene> m_ActiveScene;

    // --- Framebuffer ---
    std::shared_ptr<Framebuffer> m_ViewportFBO;

    // --- Wymiary ---
    float m_ViewportWidth = 1920.0f;
    float m_ViewportHeight = 1080.0f;

    // --- Cache pieniêdzy (aktualizowany przez MoneyChangedEvent) ---
    int         m_LastMoney = -1;
    std::string m_MoneyStr = "";
    int         m_CurrentMoney = 0;

    // --- Sk³adniki ---
    int m_CurrentTomatoes = 0;
    std::unordered_map<std::string, int> m_IngredientCounts;

    // --- Questy ---
    struct QuestData {
        std::string title;
        std::string desc;
        int         portions;
        std::string reward;
    };
    std::vector<QuestData> m_CurrentQuests;
    int m_CurrentQuestIndex = 0;

    // --- Karuzele ---
    CarouselUI m_IngredientsCarousel;
    CarouselUI m_MachinesCarousel;

    // --- Tekstury ---
    std::shared_ptr<Texture> m_CornerIcon;
    std::shared_ptr<Texture> m_TomatoIcon;
    std::shared_ptr<Texture> m_CheeseIcon;
    std::shared_ptr<Texture> m_HamIcon;
    std::shared_ptr<Texture> m_BookCloudIcon;
    std::shared_ptr<Texture> m_BookIcon;
    std::shared_ptr<Texture> m_BookStarsIcon;
    std::shared_ptr<Texture> m_BookInsideIcon;
    std::shared_ptr<Texture> m_BookXIcon;
    std::shared_ptr<Texture> m_TomatoSoupIcon;
    std::shared_ptr<Texture> m_CoinIcon;
    std::shared_ptr<Texture> m_PotIcon;
    std::shared_ptr<Texture> m_FlourIcon;
    std::shared_ptr<Texture> m_MilkIcon;
    std::shared_ptr<Texture> m_OvenIcon;
    std::shared_ptr<Texture> m_MixerIcon;
    std::shared_ptr<Texture> m_SandwichIcon;
    std::shared_ptr<Texture> m_CroissantIcon;
    std::shared_ptr<Texture> m_CupcakeIcon;

    // --- Animacje „bubbly" ---
    struct BubblyState {
        float    scale = 1.0f;
        glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    };
    std::unordered_map<std::string, BubblyState> m_BubblyStates;

    bool m_IsRecipeBookOpen = false;

    // --- Handlery eventów GLFW ---
    bool OnWindowResize(WindowResizeEvent& e);
    bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

    // --- Pomocnicze metody rysowania ---
    bool DrawBubblyImage(const std::string& id,
        const std::shared_ptr<Texture>& icon,
        glm::vec2 basePos, glm::vec2 baseSize,
        float dt, float hoverScale,
        bool darkenOnHover,
        float hitRadiusMultiplier = 0.5f,
        glm::vec4 tintColor = glm::vec4(1.0f),
        bool* outIsHovered = nullptr);

    void DrawIngredientCountText(int count,
        glm::vec2 basePos, glm::vec2 baseSize, float baseScale);

    bool DrawIngredientIcon(const std::string& id,
        const std::shared_ptr<Texture>& icon,
        glm::vec2 basePos, glm::vec2 baseSize,
        float dt, float baseScale, int count, bool showCount);

    void DrawIconWithText(const std::string& text,
        const std::shared_ptr<Texture>& iconTex,
        const glm::vec2& textPos,
        float textScale, float baseScale, float dt);

    glm::vec2 CalculateAspectSize(const std::shared_ptr<Texture>& texture,
        float targetHeight);

    void DrawRecipeIcon(const std::string& recipeId,
        const std::shared_ptr<Texture>& texture,
        glm::vec2 relativePos, float targetHeight,
        glm::vec2 bookPos, glm::vec2 bookSize, float dt);

    bool DrawScaledButton(const std::string& label,
        glm::vec2 basePos, glm::vec2 baseSize,
        float btnScale, float bsc,
        glm::vec4 colorNormal, glm::vec4 colorHover, bool hovered);

    // Rysuje trzy przyciski menu pauzy (Return / Settings / Exit)
    void DrawPauseMenu(float baseScale, float dt);

    // Rysuje panel ustawieñ graficznych (identyczny jak w MainMenuLayer)
    void DrawSettingsPanel(float baseScale, float dt);

    void DrawQuestPanel(float gameX, float gameY,
        float gameWidth, float gameHeight,
        float baseScale, bool isPlayMode);

    void DrawIngredientClouds(float gameX, float gameY,
        float gameWidth, float gameHeight,
        float baseScale, float dt);

    void DrawRecipeBook(float gameX, float gameY,
        float gameWidth, float gameHeight,
        float baseScale, float dt);
};