#pragma once
#include "../Utils/IUIPanel.h"
#include <vector>

class SettingsMenuPanel : public IUIPanel {
public:
    SettingsMenuPanel();

    virtual void OnUpdate(float dt) override;
    virtual void Draw(float baseScale) override;

    void SyncWithEngine();

private:
    int m_PendingResIndex = 0;
    int m_PendingMsaaIndex = 0;
    int m_MaxResIndex = 3; 
    std::vector<int> m_MsaaOptions = { 1, 2, 4, 8 };

    bool m_PendingMusicEnabled = true;
    bool m_PendingSoundsEnabled = true;
    bool m_PendingFullscreen = false;

    float m_BackBtnScale = 1.0f;
    float m_ApplyBtnScale = 1.0f;
    float m_ResLeftBtnScale = 1.0f;
    float m_ResRightBtnScale = 1.0f;
    float m_MsaaLeftBtnScale = 1.0f;
    float m_MsaaRightBtnScale = 1.0f;

    float m_MusicLeftBtnScale = 1.0f;
    float m_MusicRightBtnScale = 1.0f;
    float m_SoundsLeftBtnScale = 1.0f;
    float m_SoundsRightBtnScale = 1.0f;
    float m_FsLeftBtnScale = 1.0f;
    float m_FsRightBtnScale = 1.0f;

    float m_DeltaTime = 0.0f;
};