#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Core/Texture.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Events/MouseEvent.h"
#include <memory>

class MainMenuLayer : public Layer {
public:
    MainMenuLayer() : Layer("MainMenuLayer") {}
    virtual ~MainMenuLayer() = default;

    virtual void OnAttach() override;
    virtual void OnUpdate(Timestep ts) override;
    virtual void OnEvent(Event& e) override;

private:
    void PlayGame();

    std::shared_ptr<Texture> m_Background;
    float m_ViewportWidth = 1920.0f;
    float m_ViewportHeight = 1080.0f;
    bool m_IsActive = true;

    // Stany animacji przycisków (lerp skali jak w GameGuiLayer)
    float m_PlayBtnScale = 1.0f;
    float m_SettingsBtnScale = 1.0f;

    bool OnWindowResize(WindowResizeEvent& e);
};
