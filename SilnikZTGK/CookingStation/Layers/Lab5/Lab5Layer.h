#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Events/Event.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Core/Timestep.h"
#include "CookingStation/Core/Texture.h"
#include <memory>

class Lab5Layer : public Layer {
public:
    Lab5Layer() : Layer("Lab5Layer") {}
    void OnAttach() override;
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& e) override;

private:
    bool OnWindowResize(WindowResizeEvent& e);

    // tekstury do HUD-a i t³a
    std::shared_ptr<Texture> m_BgTexture;
    std::shared_ptr<Texture> m_IconTexture;

    // licznik czasu do animacji
    float m_Time = 0.0f;

    // rozmiary ekranu
    float m_ViewportWidth = 1280.0f;
    float m_ViewportHeight = 720.0f;
};