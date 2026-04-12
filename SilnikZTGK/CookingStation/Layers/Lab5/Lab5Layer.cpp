#include "CookingStation/Layers/Lab5/Lab5Layer.h"
#include "CookingStation/Layers/GuiLayer/Renderer2D.h"
#include "CookingStation/Layers/GuiLayer/Gui.h"
#include "CookingStation/Core/Input.h"
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <cmath>

void Lab5Layer::OnAttach() {
    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = (float)windowSize.first;
    m_ViewportHeight = (float)windowSize.second;
}

void Lab5Layer::OnUpdate(Timestep ts) {
    m_Time += ts;

    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Renderer2D::BeginScene(uiProj);

    // ruchome tlo
    Renderer2D::DrawQuad({ 0.0f, 0.0f }, { m_ViewportWidth, m_ViewportHeight }, { 0.25f, 0.25f, 0.28f, 1.0f });

    float scrollSpeed = 80.0f;
    float offset = fmod(m_Time * scrollSpeed, 120.0f);

    // jasne pasy w tle
    for (int i = -2; i < (int)(m_ViewportWidth / 120.0f) + 2; i++) {
        float xPos = (i * 120.0f) + offset;
        Renderer2D::DrawQuad({ xPos, 0.0f }, { 60.0f, m_ViewportHeight }, { 0.35f, 0.35f, 0.4f, 1.0f });
    }

    // --- element z przezroczystoscia ---
    Renderer2D::DrawQuad({ 20.0f, 20.0f }, { 320.0f, 130.0f }, { 0.1f, 0.2f, 0.3f, 0.4f });

    //  ramka 
    Renderer2D::DrawQuad({ 20.0f, 15.0f }, { 320.0f, 5.0f }, { 0.8f, 0.6f, 0.2f, 1.0f });

    // --- animowane ---
    float maxHpWidth = 200.0f;
    float hpPercent = (sin(m_Time * 3.0f) + 1.0f) / 2.0f;
    float currentHpWidth = maxHpWidth * hpPercent;

    // czarne t³o paska
    Renderer2D::DrawQuad({ 40.0f, 100.0f }, { maxHpWidth, 20.0f }, { 0.2f, 0.0f, 0.0f, 1.0f });
    // czerwone wype³nienie
    Renderer2D::DrawQuad({ 40.0f, 100.0f }, { currentHpWidth, 20.0f }, { 0.9f, 0.1f, 0.1f, 1.0f });

    // --- napis ---
    std::string hpText = "Zdrowie: " + std::to_string((int)(hpPercent * 100)) + "%";
    Gui::DrawGuiText(hpText, { 40.0f, 80.0f }, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });

    std::string timeText = "Czas w grze: " + std::to_string((int)m_Time) + " s";
    Gui::DrawGuiText(timeText, { 40.0f, 40.0f }, 0.45f, { 0.9f, 0.9f, 0.9f, 1.0f });

    Renderer2D::EndScene();

    glEnable(GL_DEPTH_TEST);
}

void Lab5Layer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
        return OnWindowResize(ev);
        });
}

bool Lab5Layer::OnWindowResize(WindowResizeEvent& e) {
    m_ViewportWidth = (float)e.GetWidth();
    m_ViewportHeight = (float)e.GetHeight();
    return false;
}