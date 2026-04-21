#include "HUDLayer.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Core/Input.h"
#include <glm/gtc/matrix_transform.hpp>

void HUDLayer::OnAttach() {
    Renderer2D::Init();

    // Liczymy startow¹ macierz 
    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = windowSize.first;
    m_ViewportHeight = windowSize.second;

    // Zapisujemy policzon¹ macierz do cache
    m_ProjectionMatrix = glm::ortho(0.0f, m_ViewportWidth, 0.0f, m_ViewportHeight, -1.0f, 1.0f);
}

void HUDLayer::OnUpdate(Timestep ts) {
    // 1. Zaczynamy rysowanie podaj¹c gotow¹, policzon¹ kiedyœ macierz
    Renderer2D::BeginScene(m_ProjectionMatrix);

    // Przykladowe tlo na dole ekranu
    Renderer2D::DrawQuad({ 110.0f, m_ViewportHeight - 50.0f }, { 200.0f, 30.0f }, { 0.1f, 0.1f, 0.1f, 0.8f });

    Renderer2D::EndScene();
}

void HUDLayer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) { return OnWindowResize(ev); });
}
bool HUDLayer::OnWindowResize(WindowResizeEvent& e) {
    m_ViewportWidth = (float)e.GetWidth();
    m_ViewportHeight = (float)e.GetHeight();

    // Przeliczamy i zapisujemy now¹ macierz do zmiennej klasowej
    m_ProjectionMatrix = glm::ortho(0.0f, m_ViewportWidth, 0.0f, m_ViewportHeight, -1.0f, 1.0f);

    return false; // nie blokujemy eventu (niech dotrze do edytora i Gui)
}