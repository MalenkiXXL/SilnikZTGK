#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Layers/GuiLayer/Renderer2D.h"
#include "CookingStation/Core/Timestep.h"
#include "CookingStation/Events/KeyEvent.h"
#include "CookingStation/Events/EditorEvents.h"
#include "CommandHistory.h"
#include <limits>

class EditorLayer : public Layer {
public:
    EditorLayer() : Layer("EditorLayer") {};
    ~EditorLayer() = default;

    virtual void OnAttach() override;
    virtual void OnUpdate(Timestep ts) override;
    virtual void OnEvent(Event& e) override;

    void StartPlacement(const std::string& name, const std::string& path) {
        m_PendingModelName = name;
        m_PendingModelPath = path;
        m_IsPlacing = true;
    }

    void SelectEntity(Entity entity) { m_SelectedEntity = entity; }
    Entity GetSelectedEntity() { return m_SelectedEntity; }
    void UI_Toolbar();
    void OnScenePlay();
    void OnSceneStop();

private:
    bool OnWindowResize(WindowResizeEvent& e);
    bool OnKeyPressed(KeyPressedEvent& e);
    bool OnEntityTransformChanged(EntityTransformChangedEvent& e);
    bool OnEntityDeleted(EntityDeletedEvent& e);

    // menadzer undo/redo
    CommandHistory m_CommandHistory;
    // stan zaznaczenia i stawiania
    Entity m_SelectedEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    bool m_IsPlacing = false;
    std::string m_PendingModelName = "";
    std::string m_PendingModelPath = "";

    // wymiary viewportu
    float m_ViewportWidth = 800.0f;
    float m_ViewportHeight = 600.0f;

    SceneState m_SceneState = SceneState::Edit;
    std::shared_ptr<Scene> m_EditorScene;
    std::shared_ptr<Scene> m_RuntimeScene;

    bool OnScenePlayEvent(ScenePlayEvent& e) { OnScenePlay(); return true; }
    bool OnSceneStopEvent(SceneStopEvent& e) { OnSceneStop(); return true; }
};