#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Layers/GuiLayer/Renderer2D.h"
#include "CookingStation/Core/Timestep.h"
#include "CookingStation/Events/KeyEvent.h"
#include "CookingStation/Events/EditorEvents.h"
#include "CookingStation/Renderer/ShaderLibrary.h"
#include "CommandHistory.h"
#include "CookingStation/Renderer/Framebuffer.h"
#include <memory>
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
    void SetTargetFramebuffer(const std::shared_ptr<Framebuffer>& fbo) { m_TargetFBO = fbo; }
private:
    bool OnWindowResize(WindowResizeEvent& e);
    bool OnKeyPressed(KeyPressedEvent& e);
    bool OnEntityTransformChanged(EntityTransformChangedEvent& e);
    bool OnEntityDeleted(EntityDeletedEvent& e);

    void DrawGrid(const glm::mat4& viewProj3D, const glm::vec3& camPos, float range);
    void UpdateGridPlacement(float localMouseX, float localMouseY,
        const glm::vec2& viewportSize,
        const glm::mat4& projection3D,
        const glm::mat4& view3D);

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
    ShaderLibrary m_ShaderLibrary;
    std::shared_ptr<Framebuffer> m_TargetFBO;
    

    glm::vec3 m_GridHoverPos = glm::vec3(0.0f);
    glm::vec3 m_GridEntityStartPos = glm::vec3(0.0f);

    bool OnScenePlayEvent(ScenePlayEvent& e) { OnScenePlay(); return true; }
    bool OnSceneStopEvent(SceneStopEvent& e) { OnSceneStop(); return true; }
};