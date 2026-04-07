#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Scene/SceneSerializer.h"
#include "CookingStation/Events/WindowEvent.h"
#include "GuiLayer.h"
#include "Gui.h"
#include "Renderer2D.h"


#include <limits>

class GuiLayer : public Layer {
public:
    GuiLayer() : Layer("GuiLayer") {};
    ~GuiLayer();
    virtual void OnAttach() override;
    virtual void OnUpdate(Timestep ts) override;
    virtual void OnEvent(Event& e) override;
    void SetScene(std::shared_ptr<Scene> scene) { m_ActiveScene = scene; }
    bool OnWindowResize(WindowResizeEvent& e);


protected:
    std::shared_ptr<Scene> m_ActiveScene;

private:
    Entity m_SelectedEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    float m_ViewportWidth = 800.0f;
    float m_ViewportHeight = 600.0f;
    std::string m_PendingModelPath = "";
    std::string m_PendingModelName = "";
    bool m_IsPlacing = false;
};