#include "HUDLayer.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Core/Input.h"
#include <glm/gtc/matrix_transform.hpp>

void HUDLayer::OnAttach() {
    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = windowSize.first;
    m_ViewportHeight = windowSize.second;

    m_ProjectionMatrix = glm::ortho(0.0f, m_ViewportWidth, 0.0f, m_ViewportHeight, -1.0f, 1.0f);
}

void HUDLayer::OnUpdate(Timestep ts) {
    Renderer2D::BeginScene(m_ProjectionMatrix);
    Renderer2D::EndScene();
}

void HUDLayer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) { return OnWindowResize(ev); });
}
bool HUDLayer::OnWindowResize(WindowResizeEvent& e) {
    m_ViewportWidth = (float)e.GetWidth();
    m_ViewportHeight = (float)e.GetHeight();

    m_ProjectionMatrix = glm::ortho(0.0f, m_ViewportWidth, 0.0f, m_ViewportHeight, -1.0f, 1.0f);

    return false; 
}