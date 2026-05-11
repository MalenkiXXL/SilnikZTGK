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
};