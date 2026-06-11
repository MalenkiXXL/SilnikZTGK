#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Scene/Scene.h"
#include "Camera.h"
#include "CookingStation/Events/Event.h"
#include "CookingStation/Events/MouseEvent.h"
#include "CookingStation/Core/Timestep.h"
#include "CookingStation/Events/KeyEvent.h"
#include "CookingStation/Events/GameEvents.h" 

class CameraLayer : public Layer
{
public:
    CameraLayer();
    ~CameraLayer();
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& event) override;

private:
    bool OnMouseScrolled(MouseScrolledEvent& e);
    bool m_IsTopDown = false;
    bool OnKeyPressed(KeyPressedEvent& e);
    Camera m_Camera;

    bool m_Panning = false;
    float m_LastMouseX = 0.0f;
    float m_LastMouseY = 0.0f;

    bool m_IsGamePaused = false;
    std::size_t m_GamePausedSubId = 0;
    std::size_t m_GameResumedSubId = 0;
};