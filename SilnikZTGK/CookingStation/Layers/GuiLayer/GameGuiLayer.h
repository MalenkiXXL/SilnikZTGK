#pragma once
#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Events/MouseEvent.h"
#include "CookingStation/Core/Timestep.h"
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "CookingStation/Core/Texture.h"
#include <memory>
#include "CookingStation/Renderer/Framebuffer.h"

class GameGuiLayer : public Layer {
public:
    GameGuiLayer() : Layer("GameGuiLayer") {}
    virtual ~GameGuiLayer() = default;

    virtual void OnAttach() override;
    virtual void OnUpdate(Timestep ts) override;
    virtual void OnEvent(Event& e) override;
    void SetViewportFramebuffer(const std::shared_ptr<Framebuffer>& fbo) { m_ViewportFBO = fbo; }
    void ReloadQuests();
    static bool s_NeedsQuestReload;
private:
    bool OnWindowResize(WindowResizeEvent& e);
    bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

    float m_ViewportWidth = 1920.0f;
    float m_ViewportHeight = 1080.0f;
    std::shared_ptr<Framebuffer> m_ViewportFBO;

    struct QuestData {
        std::string title;
        std::string desc;
        int portions;
        std::string reward;
    };

    std::vector<QuestData> m_CurrentQuests;
    int m_CurrentQuestIndex = 0;
    std::shared_ptr<Texture> m_CornerIcon;
    std::shared_ptr<Texture> m_TomatoIcon;
    std::shared_ptr<Texture> m_BookCloudIcon;
    std::shared_ptr<Texture> m_BookIcon;
    std::shared_ptr<Texture> m_BookStarsIcon;
    std::shared_ptr<Texture> m_BookInsideIcon;
    std::shared_ptr<Texture> m_BookXIcon;
    std::shared_ptr<Texture> m_TomatoSoupIcon;

    struct BubblyState {
        float scale = 1.0f;
        glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    std::unordered_map<std::string, BubblyState> m_BubblyStates;

    bool DrawBubblyImage(const std::string& id, const std::shared_ptr<Texture>& icon, glm::vec2 basePos,
                         glm::vec2 baseSize, float dt, float hoverScale, bool darkenOnHover,
                         float hitRadiusMultiplier = 0.5f, glm::vec4 tintColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                         bool* outIsHovered = nullptr);

    void DrawIngredientCountText(int count, glm::vec2 basePos, glm::vec2 baseSize, float baseScale);

    bool DrawIngredientIcon(const std::string& id, const std::shared_ptr<Texture>& icon,
                            glm::vec2 basePos, glm::vec2 baseSize, float dt,
                            float baseScale, int count, bool showCount);

    bool m_IsRecipeBookOpen = false;

    glm::vec2 CalculateAspectSize(const std::shared_ptr<Texture>& texture, float targetHeight);

    void DrawRecipeIcon(const std::string& recipeId, const std::shared_ptr<Texture>& texture,
        glm::vec2 relativePos, float targetHeight,
        glm::vec2 bookPos, glm::vec2 bookSize, float dt);
    void DrawQuestPanel(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, bool isPlayMode);
    void DrawIngredientClouds(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt);
    void DrawRecipeBook(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt);
};