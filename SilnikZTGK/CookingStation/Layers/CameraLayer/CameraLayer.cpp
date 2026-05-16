#include "CameraLayer.h"
#include "../../Core/Input.h"
#include <iostream>
#include <GLFW/glfw3.h>
#include "CookingStation/Layers/GuiLayer/Gui.h"
#include "CookingStation/Core/Timestep.h"
#include "CookingStation/Scene/SceneSerializer.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Core/Application.h"


// Izometryczny kat kamery 
//
//  Yaw = -135
//  Pitch = -30
//
//  Pozycja startowa: odsuni�ta od poczatku swiata w kierunku odwrotnym do Front,
//  �eby scena byla od razu widoczna.
//
//  Sterowanie:
//    WASD  - przesuwa kamere wzdluz wektorow Right/Up kamery (pan po ekranie)
//    Scroll - zoom in/out (zmienia OrthoSize)
// 

// K�ty izometryczne
static constexpr float ISO_YAW = -135.0f;
static constexpr float ISO_PITCH = -35.0f;

// Predkosc przesuniecia (WASD)
static constexpr float PAN_SPEED = 10.0f;
static constexpr float LERP_SPEED = 4.0f;

CameraLayer::CameraLayer() : Layer("CameraLayer"),
                             m_Camera(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f), ISO_YAW, ISO_PITCH) {
};

CameraLayer::~CameraLayer() {};

void CameraLayer::OnUpdate(Timestep ts) {
    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    if (activeScene) {
        activeScene->SetCamera(&m_Camera);
    }

    m_Camera.UpdateLerp((float) ts, LERP_SPEED);

    if (Gui::AnyItemActive()) return;

    // --- RMB Pan ---
    auto [mouseX, mouseY] = Input::GetMousePosition();

    // 1. OBLICZAMY FAKTYCZNY ROZMIAR EKRANU GRY (na podstawie GUI)
    float viewportW = (float) activeScene->GetViewportWidth();
    float viewportH = (float) activeScene->GetViewportHeight();

    // Zabezpieczenie przed minimalizacją
    if (viewportW <= 0.0f || viewportH <= 0.0f) return;

    if (Input::IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        m_Panning = true;
        m_LastMouseX = mouseX;
        m_LastMouseY = mouseY;

        m_Camera.TargetPosition = m_Camera.Position;
    }

    if (Input::IsMouseButtonJustReleased(GLFW_MOUSE_BUTTON_RIGHT)) {
        m_Panning = false;
    }

    if (m_Panning) {
        float dx = mouseX - m_LastMouseX;
        float dy = mouseY - m_LastMouseY;
        m_LastMouseX = mouseX;
        m_LastMouseY = mouseY;

        // 2. LICZYMY PROPORCJE WZGLĘDEM VIEWPORTU GRY, A NIE CAŁEGO OKNA
        float currentAspect = viewportW / viewportH;

        float worldH = m_Camera.OrthoSize * 2.0f;
        float worldW = worldH * currentAspect;

        // 3. UŻYWAMY VIEWPORTU DO WYLICZENIA UŁAMKA PRZESUNIĘCIA
        float worldDx = (dx / viewportW) * worldW;
        float worldDy = (dy / viewportH) * worldH;

        glm::vec3 delta = -m_Camera.Right * worldDx + m_Camera.Up * worldDy;
        m_Camera.Position += delta;
        m_Camera.TargetPosition += delta;
    }

    // --- WASD ---
    glm::vec3 dir(0.0f);

    if (Input::IsKeyPressed(GLFW_KEY_W)) dir += m_Camera.Up;
    if (Input::IsKeyPressed(GLFW_KEY_S)) dir -= m_Camera.Up;
    if (Input::IsKeyPressed(GLFW_KEY_D)) dir += m_Camera.Right;
    if (Input::IsKeyPressed(GLFW_KEY_A)) dir -= m_Camera.Right;

    if (glm::length(dir) > 0.0f) {
        dir = glm::normalize(dir);
        m_Camera.TargetPosition += dir * PAN_SPEED * (float) ts;
    }
}

void CameraLayer::OnEvent(Event &event) {
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<MouseScrolledEvent>(
            [this](MouseScrolledEvent &e) { return OnMouseScrolled(e); }
    );

    dispatcher.Dispatch<KeyPressedEvent>(
            [this](KeyPressedEvent &e) { return OnKeyPressed(e); }
    );
}

bool CameraLayer::OnMouseScrolled(MouseScrolledEvent &e) {
    if (Gui::AnyItemActive()) return false;

    // ProcessMouseScroll teraz zmienia OrthoSize zamiast FOV
    m_Camera.ProcessMouseScroll((float) e.GetYOffset());
    return false; // nie pochlaniac eventu - inne warstwy tez moga go potrzebowac
}

bool CameraLayer::OnKeyPressed(KeyPressedEvent& e) {
    if (Gui::AnyItemActive()) return false;

    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    if (!activeScene) return false;

    if (activeScene->GetState() == SceneState::Edit) {

        if (e.GetKeyCode() == GLFW_KEY_T) {
            m_IsTopDown = !m_IsTopDown;

            if (m_IsTopDown) {
                m_Camera.Pitch = -89.9f;
                m_Camera.Yaw = -90.0f;
            } else {
                m_Camera.Pitch = ISO_PITCH;
                m_Camera.Yaw = ISO_YAW;
            }

            m_Camera.TargetPosition = m_Camera.Position;

            m_Camera.Pitch = m_Camera.Pitch;
            m_Camera.Yaw = m_Camera.Yaw;

            m_Camera.updateCameraVectors();

            return true;
        }
    }

    return false;
}