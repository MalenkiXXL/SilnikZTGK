#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Layers/GuiLayer/Renderer2D.h"
#include <limits>

class EditorLayer : public Layer {
public:
    EditorLayer() : Layer("EditorLayer") {};
    ~EditorLayer() = default;

    virtual void OnAttach() override;
    virtual void OnUpdate() override;
    virtual void OnEvent(Event& e) override;

    // Funkcje do komunikacji z GuiLayer
    void SetScene(std::shared_ptr<Scene> scene) { m_ActiveScene = scene; }
    void StartPlacement(const std::string& name, const std::string& path) {
        m_PendingModelName = name;
        m_PendingModelPath = path;
        m_IsPlacing = true;
    }

    void SelectEntity(Entity entity) { m_SelectedEntity = entity; }
    Entity GetSelectedEntity() { return m_SelectedEntity; }

private:
    bool OnWindowResize(WindowResizeEvent& e);

    std::shared_ptr<Scene> m_ActiveScene;

    // Stan zaznaczenia i stawiania
    Entity m_SelectedEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    bool m_IsPlacing = false;
    std::string m_PendingModelName = "";
    std::string m_PendingModelPath = "";

    // Wymiary viewportu
    float m_ViewportWidth = 800.0f;
    float m_ViewportHeight = 600.0f;
};