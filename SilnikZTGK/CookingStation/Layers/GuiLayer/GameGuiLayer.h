#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Events/MouseEvent.h"
#include "CookingStation/Core/Timestep.h"
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Core/Texture.h"
#include "Panels/PauseMenuPanel.h"
#include "CookingStation/Renderer/Framebuffer.h"
#include "CarouselUI.h"
#include "CookingStation/Scripts/Managers/IngredientType.h"
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "CookingStation/Tools/QuestGenerator/QuestManager.h"

class Scene;

struct BubblyState {
    float scale = 1.0f;
    glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
};

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

private:
    bool OnWindowResize(WindowResizeEvent& e);
    bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

    std::shared_ptr<Texture> GetIconForIngredient(IngredientType type);
    std::string GetModelPathForIngredient(IngredientType type);
    std::string GetIngredientName(IngredientType type);
    void LoadQuestsFromFile(const std::string& filepath);
    void CheckQuestProgress();

    bool DrawBubblyImage(const std::string& id, const std::shared_ptr<Texture>& icon,
        glm::vec2 basePos, glm::vec2 baseSize, float dt,
        float hoverScale = 1.15f, bool darkenOnHover = false,
        float hitRadiusMultiplier = 0.5f,
        glm::vec4 tintColor = { 1.0f, 1.0f, 1.0f, 1.0f },
        bool* outIsHovered = nullptr);

    bool DrawIngredientIcon(const std::string& id, const std::shared_ptr<Texture>& icon,
        glm::vec2 basePos, glm::vec2 baseSize,
        float dt, float baseScale, int count, bool showCount);

    void DrawIngredientCountText(int count, glm::vec2 basePos, glm::vec2 baseSize, float baseScale);

    void DrawRecipeIcon(const std::string& recipeId, const std::shared_ptr<Texture>& texture,
        glm::vec2 relativePct, float targetHeight,
        glm::vec2 bookPos, glm::vec2 bookSize, float dt);

    void DrawIconWithText(const std::string& text, const std::shared_ptr<Texture>& iconTex,
        const glm::vec2& textPos, float textScale, float baseScale, float dt);

    void DrawQuestPanel(float gameX, float gameY, float gameWidth, float gameHeight,
        float baseScale, bool isPlayMode);

    void DrawIngredientClouds(float gameX, float gameY, float gameWidth, float gameHeight,
        float baseScale, float dt);

    void DrawRecipeBook(float gameX, float gameY, float gameWidth, float gameHeight,
        float baseScale, float dt);

    void DrawOrderTickets(float gameX, float gameY, float gameWidth, float gameHeight,
        float baseScale);

    void DrawCustomerOrders(float gameX, float gameY, float gameWidth, float gameHeight,
        float baseScale);

    void DrawHoverCloudUI(const glm::vec2& screenPos, const std::shared_ptr<Texture>& icon, int amount, float baseScale);
    void DrawCrateHoverInfo(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt);

private:
    bool m_IsVisible = false;
    bool m_IsActive = false;
    std::shared_ptr<Scene>       m_ActiveScene;
    std::shared_ptr<Framebuffer> m_ViewportFBO;
    float m_ViewportWidth = 1920.0f;
    float m_ViewportHeight = 1080.0f;

    std::unique_ptr<PauseMenuPanel> m_PausePanel;

    CarouselUI m_IngredientsCarousel;
    CarouselUI m_MachinesCarousel;

    std::shared_ptr<Texture> m_HeartIcon;
    std::shared_ptr<Texture> m_StarIcon;
    std::shared_ptr<Texture> m_CoinIcon;
    std::shared_ptr<Texture> m_CoinCloudIcon;
    std::shared_ptr<Texture> m_ClockIcon;
    std::shared_ptr<Texture> m_QuestionMarkIcon;
    std::shared_ptr<Texture> m_ExclamationIcon;
    std::shared_ptr<Texture> m_RecipeBookIcon;
    std::shared_ptr<Texture> m_RecipeBookOpenBg;
    std::shared_ptr<Texture> m_EventsIcon;
    std::shared_ptr<Texture> m_PauseIcon;

    std::shared_ptr<Texture> m_CornerIcon;

    std::shared_ptr<Texture> m_TomatoIcon;
    std::shared_ptr<Texture> m_CheeseIcon;
    std::shared_ptr<Texture> m_HamIcon;
    std::shared_ptr<Texture> m_MilkIcon;
    std::shared_ptr<Texture> m_FlourIcon;
    std::shared_ptr<Texture> m_PotIcon;
    std::shared_ptr<Texture> m_OvenIcon;
    std::shared_ptr<Texture> m_MixerIcon;

    std::shared_ptr<Texture> m_BookCloudIcon;
    std::shared_ptr<Texture> m_BookIcon;
    std::shared_ptr<Texture> m_BookStarsIcon;
    std::shared_ptr<Texture> m_BookInsideIcon;
    std::shared_ptr<Texture> m_BookXIcon;

    std::shared_ptr<Texture> m_TomatoSoupIcon;
    std::shared_ptr<Texture> m_SandwichIcon;
    std::shared_ptr<Texture> m_CupcakeIcon;
    std::shared_ptr<Texture> m_CroissantIcon;

    std::shared_ptr<Texture> m_CustomerOrderTex;
    std::shared_ptr<Texture> m_HelperOrderTex;

    std::size_t m_SubMoney = 0;
    std::size_t m_SubInventory = 0;
    std::size_t m_SubDrag = 0;

    std::size_t m_GameStartedSubId = 0;
    std::size_t m_InventorySubId = 0;
    std::size_t m_MoneySubId = 0;
    std::size_t m_OrderTakenSubId = 0;

    std::unordered_map<IngredientType, int>   m_Inventory;
    std::unordered_map<IngredientType, float> m_ItemScales;
    glm::vec4 m_InventoryRect = { 0.0f, 0.0f, 0.0f, 0.0f };

    bool m_IsDragging = false;
    IngredientType m_DraggedType;
    std::shared_ptr<Texture> m_DraggedIcon;
    std::string m_DraggedModelPath;

    int m_CurrentTomatoes = 0;
    std::unordered_map<std::string, int> m_IngredientCounts;

    int         m_CurrentMoney = 0;
    int         m_LastMoney = -1;
    float       m_MoneyScale = 1.0f;
    std::string m_MoneyStr = "0";

    std::vector<Entity> m_ActiveOrderTickets;

    bool m_IsRecipeBookOpen = false;
    int  m_CurrentRecipePage = 0;
    bool m_IsEventsPanelOpen = false;


    bool m_ShowFPS = false;

    float m_RecipeBtnScale = 1.0f;
    float m_EventsBtnScale = 1.0f;
    float m_PauseBtnScale = 1.0f;
    float m_LeftArrowScale = 1.0f;
    float m_RightArrowScale = 1.0f;
    float m_CloseBtnScale = 1.0f;
    float m_EvCloseBtnScale = 1.0f;

    std::unordered_map<std::string, BubblyState> m_BubblyStates;

    std::size_t m_GamePausedSubId = 0;
    std::size_t m_GameResumedSubId = 0;
    bool m_IsGamePaused = false;

    bool  m_IsBuildModeActive = false;
    float m_BuildPanelSlideY = 0.0f;   
    int   m_HeldMachineIndex = -1;     
    bool m_JustSelectedFromPanel = false;

    struct MachineEntry {
        std::string              Label;
        std::string              PrefabPath;
        std::shared_ptr<Texture> Icon;
    };

    std::vector<MachineEntry> m_MachineEntries;
    std::vector<std::pair<Entity, glm::vec3>> m_PreviewGroup;
    Entity m_MovingMachineEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    glm::vec3 m_MovingMachineOriginalPos = glm::vec3(0.0f);
    std::vector<std::pair<Entity, glm::vec3>> m_MovingGroup;

    void DrawPackageHoverInfo(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt);
    void DrawBuildModeButton(float gameX, float gameY, float gameWidth, float gameHeight,
        float baseScale, float dt);
    void DrawBuildModePanel(float gameX, float gameY, float gameWidth, float gameHeight,
        float baseScale, float dt);
    void UpdateBuildModePlacement();
    void ActivateBuildMode();
    void DeactivateBuildMode();
    void DrawBuildModeOverlay(float baseScale);
    void DrawBuildGrid(const glm::mat4& viewProj3D, const glm::vec3& camPos,
        const glm::vec3& hoverPos);
};