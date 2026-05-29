#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Core/Texture.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Events/MouseEvent.h"
#include "CookingStation/Core/Timestep.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

class MainMenuLayer : public Layer {
public:
    MainMenuLayer() : Layer("MainMenuLayer") {}
    virtual ~MainMenuLayer() = default;

    virtual void OnAttach() override;
    virtual void OnUpdate(Timestep ts) override;
    virtual void OnEvent(Event& e) override;

private:
    void PlayGame();
    void DrawMainMenu(float baseScale, float dt);
    void DrawSettingsPanel(float baseScale, float dt);

    bool DrawScaledButton(const std::string& label,
        glm::vec2 basePos, glm::vec2 baseSize,
        float btnScale, float baseScale_,
        glm::vec4 colorNormal, glm::vec4 colorHover,
        bool hovered);

    std::shared_ptr<Texture> m_Background;
    float m_ViewportWidth = 1920.0f;
    float m_ViewportHeight = 1080.0f;
    bool  m_IsActive = true;

    float m_PlayBtnScale = 1.0f;
    float m_SettingsBtnScale = 1.0f;

    bool  m_SettingsOpen = false;

    float m_BackBtnScale = 1.0f;
    float m_ApplyBtnScale = 1.0f;
    float m_ResLeftBtnScale = 1.0f;
    float m_ResRightBtnScale = 1.0f;
    float m_MsaaLeftBtnScale = 1.0f;
    float m_MsaaRightBtnScale = 1.0f;

    int m_PendingResIndex = 0;
    int m_PendingMsaaIndex = 0;

    static constexpr int MsaaOptions[] = { 1, 2, 4, 8 };
    static constexpr int MsaaOptionCount = 4;

    bool OnWindowResize(WindowResizeEvent& e);
};