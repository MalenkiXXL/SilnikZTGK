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
#include "Panels/BuildModePanel.h"
#include "Panels/RecipeBookPanel.h"
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

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

    void DrawIconWithText(const std::string& text, const std::shared_ptr<Texture>& iconTex,
        const glm::vec2& textPos, float textScale, float baseScale, float dt);

    void DrawQuestPanel(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, bool isPlayMode);
    void DrawIngredientClouds(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt);
    void DrawOrderTickets(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale);
    void DrawCustomerOrders(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale);
    void DrawHoverCloudUI(const glm::vec2& screenPos, const std::shared_ptr<Texture>& icon, int amount, float baseScale);
    void DrawCrateHoverInfo(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt);
    void DrawPackageHoverInfo(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt);

private:
    bool m_IsVisible = false;
    bool m_IsActive = false;
    std::shared_ptr<Scene>       m_ActiveScene;
    std::shared_ptr<Framebuffer> m_ViewportFBO;
    float m_ViewportWidth = 1920.0f;
    float m_ViewportHeight = 1080.0f;

    std::unique_ptr<PauseMenuPanel> m_PausePanel;
    BuildModePanel m_BuildModePanel;
    RecipeBookPanel m_RecipeBookPanel;

    CarouselUI m_IngredientsCarousel;
    CarouselUI m_MachinesCarousel;

    std::shared_ptr<Texture> m_CoinIcon;
    std::shared_ptr<Texture> m_CoinCloudIcon;
    std::shared_ptr<Texture> m_CornerIcon;
    std::shared_ptr<Texture> m_TomatoIcon;
    std::shared_ptr<Texture> m_CheeseIcon;
    std::shared_ptr<Texture> m_HamIcon;
    std::shared_ptr<Texture> m_MilkIcon;
    std::shared_ptr<Texture> m_FlourIcon;
    std::shared_ptr<Texture> m_QuestionMarkIcon;
    std::shared_ptr<Texture> m_CustomerOrderTex;
    std::shared_ptr<Texture> m_HelperOrderTex;
    std::shared_ptr<Texture> m_BookCloudIcon;

    std::size_t m_GameStartedSubId = 0;
    std::size_t m_InventorySubId = 0;
    std::size_t m_MoneySubId = 0;
    std::size_t m_OrderTakenSubId = 0;
    std::size_t m_GamePausedSubId = 0;
    std::size_t m_GameResumedSubId = 0;

    std::unordered_map<std::string, int> m_IngredientCounts;
    std::unordered_map<std::string, BubblyState> m_BubblyStates;

    int m_CurrentTomatoes = 0;
    int m_CurrentMoney = 0;
    int m_LastMoney = -1;
    std::string m_MoneyStr = "0";

    std::vector<Entity> m_ActiveOrderTickets;
    bool m_ShowFPS = false;
    bool m_IsGamePaused = false;
};