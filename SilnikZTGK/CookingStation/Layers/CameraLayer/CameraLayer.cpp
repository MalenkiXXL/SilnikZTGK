#include "CameraLayer.h"
#include "../../Core/Input.h"
#include <iostream>
#include <GLFW/glfw3.h>
#include "CookingStation/Layers/GuiLayer/Gui.h"
#include "CookingStation/Core/Timestep.h"
#include "CookingStation/Scene/SceneSerializer.h"
#include "CookingStation/Scene/SceneManager.h"


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

    // Poruszamy sie wzdluz Right i Up kamery (nie Forward!) - to daje
    // "przesuwanie widoku" bez zmiany k�ta patrzenia (klasyczny pan izometryczny).
    //
    // W efekcie:
    //   W / S    przesuwa widok w gore / dol ekranu
    //   A / D   przesuwa widok w lewo / prawo ekranu

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
}

bool CameraLayer::OnMouseScrolled(MouseScrolledEvent &e) {
    if (Gui::AnyItemActive()) return false;

    // ProcessMouseScroll teraz zmienia OrthoSize zamiast FOV
    m_Camera.ProcessMouseScroll((float) e.GetYOffset());
    return false; // nie pochlaniac eventu - inne warstwy tez moga go potrzebowac
}
