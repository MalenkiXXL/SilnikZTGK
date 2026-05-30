#pragma once
#include "../Utils/IUIPanel.h"
#include <vector>

class SettingsMenuPanel : public IUIPanel {
public:
    SettingsMenuPanel();

    virtual void OnUpdate(float dt) override;
    virtual void Draw(float baseScale) override;

    // Metoda wywoływana przy otwarciu panelu, żeby zaciągnąć aktualne ustawienia silnika
    void SyncWithEngine();

private:
    int m_PendingResIndex = 0;
    int m_PendingMsaaIndex = 0;
    std::vector<int> m_MsaaOptions = { 1, 2, 4, 8 };

    // Animacje skali przycisków
    float m_BackBtnScale = 1.0f;
    float m_ApplyBtnScale = 1.0f;
    float m_ResLeftBtnScale = 1.0f;
    float m_ResRightBtnScale = 1.0f;
    float m_MsaaLeftBtnScale = 1.0f;
    float m_MsaaRightBtnScale = 1.0f;

    float m_DeltaTime = 0.0f; // Cache dt dla funkcji rysujących
};