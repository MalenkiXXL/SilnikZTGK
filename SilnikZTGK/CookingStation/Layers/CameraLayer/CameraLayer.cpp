#include "CameraLayer.h"
#include "../../Core/Input.h"
#include <iostream>
#include <GLFW/glfw3.h>
#include "CookingStation/Layers/GuiLayer/Utils/Gui.h"
#include "CookingStation/Core/Timestep.h"
#include "CookingStation/Scene/SceneSerializer.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Core/Application.h"

static constexpr float ISO_YAW = -135.0f;
static constexpr float ISO_PITCH = -35.0f;

static constexpr float PAN_SPEED = 10.0f;
static constexpr float LERP_SPEED = 4.0f;

CameraLayer::CameraLayer() : Layer("CameraLayer"),
m_Camera(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f), ISO_YAW, ISO_PITCH)
{
    auto& appBus = Application::Get().GetEventBus();

    m_GamePausedSubId = appBus.Subscribe<GamePausedEvent>([this](const GamePausedEvent&) {
        m_IsGamePaused = true;
        });

    m_GameResumedSubId = appBus.Subscribe<GameResumedEvent>([this](const GameResumedEvent&) {
        m_IsGamePaused = false;
        });
}

CameraLayer::~CameraLayer()
{
    auto& appBus = Application::Get().GetEventBus();
    if (m_GamePausedSubId != 0) appBus.Unsubscribe<GamePausedEvent>(m_GamePausedSubId);
    if (m_GameResumedSubId != 0) appBus.Unsubscribe<GameResumedEvent>(m_GameResumedSubId);
}

void CameraLayer::OnUpdate(Timestep ts) {
    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    if (activeScene) {
        activeScene->SetCamera(&m_Camera);
    }

    m_Camera.UpdateLerp((float) ts, LERP_SPEED);

    if (m_IsGamePaused) return;

    if (Gui::AnyItemActive()) return;

    auto [mouseX, mouseY] = Input::GetMousePosition();

    float viewportW = (float) activeScene->GetViewportWidth();
    float viewportH = (float) activeScene->GetViewportHeight();

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

    glm::vec3 oldTarget = m_Camera.TargetPosition;

    if (m_Panning) {
        float dx = mouseX - m_LastMouseX;
        float dy = mouseY - m_LastMouseY;
        m_LastMouseX = mouseX;
        m_LastMouseY = mouseY;

        float currentAspect = viewportW / viewportH;

        float worldH = m_Camera.OrthoSize * 2.0f;
        float worldW = worldH * currentAspect;

        float worldDx = (dx / viewportW) * worldW;
        float worldDy = (dy / viewportH) * worldH;

        glm::vec3 delta = -m_Camera.Right * worldDx + m_Camera.Up * worldDy;
        m_Camera.Position += delta;
        m_Camera.TargetPosition += delta;
    }

    glm::vec3 dir(0.0f);

    if (Input::IsKeyPressed(GLFW_KEY_W)) dir += m_Camera.Up;
    if (Input::IsKeyPressed(GLFW_KEY_S)) dir -= m_Camera.Up;
    if (Input::IsKeyPressed(GLFW_KEY_D)) dir += m_Camera.Right;
    if (Input::IsKeyPressed(GLFW_KEY_A)) dir -= m_Camera.Right;

    if (glm::length(dir) > 0.0f) {
        dir = glm::normalize(dir);
        m_Camera.TargetPosition += dir * PAN_SPEED * (float) ts;
    }

#ifdef CS_DISTRIBUTION
    m_Camera.ApplyBounds();
#else
    if (activeScene && activeScene->GetState() == SceneState::Play)
    {
        m_Camera.ApplyBounds();
    }
#endif
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
    if (m_IsGamePaused) return false;
    if (Gui::AnyItemActive()) return false;

    m_Camera.ProcessMouseScroll((float) e.GetYOffset());
    return false; 
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