#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Scene/SceneSerializer.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Events/WindowEvent.h"
#include "GuiLayer.h"
#include "Gui.h"
#include "Renderer2D.h"
#include "CookingStation/Renderer/Framebuffer.h"
#include <memory>
#include <limits>

class GuiLayer : public Layer {
public:
    GuiLayer() : Layer("GuiLayer") {};
    ~GuiLayer();
    virtual void OnAttach() override;
    virtual void OnUpdate(Timestep ts) override;
    virtual void OnEvent(Event& e) override;
    bool OnWindowResize(WindowResizeEvent& e);
    void SetViewportFramebuffer(const std::shared_ptr<Framebuffer>& fbo) { m_ViewportFBO = fbo; }

private:
    Entity m_SelectedEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    float m_ViewportWidth = 800.0f;
    float m_ViewportHeight = 600.0f;
    float m_StatsUpdateTimer = 1.0f;

    // Zbuforowane teksty dla interfejsu
    std::string m_FpsText = "FPS: 0";
    std::string m_FrameTimeText = "Frame Time: 0.00 ms";
    std::string m_CpuText = "CPU Logika: 0.00 ms";
    std::string m_GpuText = "GPU Render: 0.00 ms";
    std::string m_DrawCalls3DText = "Draw Calls (3D): 0";
    std::string m_Tris3DText = "Trojkaty (3D): 0";
    std::string m_DrawCallsUIText = "Draw Calls (UI): 0";
    std::string m_TrisUIText = "Trojkaty (UI): 0";
    std::string m_PendingModelPath = "";
    std::string m_PendingModelName = "";

    bool m_IsPlacing = false;
    bool m_ShowFileMenu = false;
    bool m_ShowViewMenu = false;
    bool m_ShowEnvironmentPanel = false;
    bool m_ShowHierarchyPanel = false;
    bool m_ShowLibraryPanel = true;
    bool m_ShowInspectorPanel = true;
    bool m_ShowDiagnosticPanel = false; 
    bool m_ShowSaveDialog = false;
    bool m_ShowLoadDialog = false;
    std::string m_SaveFileName = "moja_scena";
    std::string m_LoadFileName = "moja_scena";
    bool m_IsDraggingTransform = false;
    glm::vec3 m_TransformStartPos = { 0.0f, 0.0f, 0.0f };
    std::shared_ptr<Framebuffer> m_ViewportFBO;
};