#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Scene/Scene.h"
#include "Camera.h"
#include "CookingStation/Events/Event.h"
#include "CookingStation/Events/MouseEvent.h"
#include "CookingStation/Core/Timestep.h"

class CameraLayer : public Layer
{
public:
    CameraLayer();
    ~CameraLayer();
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& event) override;

private:
    bool OnMouseScrolled(MouseScrolledEvent& e);
    Camera m_Camera;
};