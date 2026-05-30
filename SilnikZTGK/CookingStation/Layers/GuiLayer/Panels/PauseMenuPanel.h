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

    // Żeby z zewnątrz powiadomić o wciśnięciu ESC
    void TogglePause();
    bool IsPaused() const { return m_IsPaused; }

private:
    bool m_IsPaused = false;
    float m_DeltaTime = 0.0f;

    float m_ReturnBtnScale = 1.0f;
    float m_SettingsBtnScale = 1.0f;
    float m_ExitBtnScale = 1.0f;

    // Panel ustawień jest "Dzieckiem" panelu pauzy
    std::unique_ptr<SettingsMenuPanel> m_SettingsPanel;
};