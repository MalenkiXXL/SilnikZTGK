#pragma once
#include "../Utils/IUIPanel.h"
#include "SettingsMenuPanel.h"
#include <memory>

class PauseMenuPanel : public IUIPanel {
public:
    PauseMenuPanel();

    virtual void OnUpdate(float dt) override;
    virtual void Draw(float baseScale) override;
    virtual void OnEvent(Event& e) override;

    void TogglePause();
    bool IsPaused() const { return m_IsPaused; }

private:
    bool m_IsPaused = false;

    bool m_IsBuildMode = false;

    float m_DeltaTime = 0.0f;

    float m_SettingsBtnCarrotScale = 1.0f;
    float m_ResumeBtnScale = 1.0f;
    float m_MenuBtnScale = 1.0f;

    std::unique_ptr<SettingsMenuPanel> m_SettingsPanel;
};