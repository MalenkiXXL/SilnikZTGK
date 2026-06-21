#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Core/Texture.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Events/MouseEvent.h"
#include "CookingStation/Core/Timestep.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <cstddef>

class SettingsMenuPanel;
class CreditsPanel;
class MainMenuLayer : public Layer {
public:
    MainMenuLayer() : Layer("MainMenuLayer") {}
    virtual ~MainMenuLayer() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(Timestep ts) override;
    virtual void OnEvent(Event& e) override;
    bool m_IsActive = true;

private:
    void PlayGame();
    void DrawMainMenu(float baseScale, float dt);
    void DrawSettingsPanel(float baseScale, float dt);

    bool DrawImageButton(const std::shared_ptr<Texture>& tex, glm::vec2 basePos, glm::vec2 baseSize, float btnScale, float baseScale_, bool hovered);

    bool DrawScaledButton(const std::string& label,
        glm::vec2 basePos, glm::vec2 baseSize,
        float btnScale, float baseScale_,
        glm::vec4 colorNormal, glm::vec4 colorHover,
        bool hovered);

    bool OnWindowResize(WindowResizeEvent& e);

    std::shared_ptr<Texture> m_Background;
    std::shared_ptr<Texture> m_BoardTex;

    std::shared_ptr<Texture> m_PlayBtnTex;
    std::shared_ptr<Texture> m_SettingsBtnTex;
    std::shared_ptr<Texture> m_CreditsBtnTex;
    std::shared_ptr<Texture> m_ExitBtnTex;

    float m_ViewportWidth = 1920.0f;
    float m_ViewportHeight = 1080.0f;

    float m_PlayBtnScale = 1.0f;
    float m_SettingsBtnScale = 1.0f;
    float m_CreditsBtnScale = 1.0f;
    float m_ExitBtnScale = 1.0f;

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

    std::shared_ptr<SettingsMenuPanel> m_SettingsPanel;
    std::shared_ptr<CreditsPanel> m_CreditsPanel;

    std::size_t m_ShowMenuSubId = 0;


    float m_MusicLeftBtnScale = 1.0f;
    float m_MusicRightBtnScale = 1.0f;
    bool  m_PendingMusicEnabled = true;

    float m_SoundsLeftBtnScale = 1.0f;
    float m_SoundsRightBtnScale = 1.0f;
    bool  m_PendingSoundsEnabled = true;
};