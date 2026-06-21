#pragma once
#include "CookingStation/Core/Texture.h"
#include "CookingStation/Events/Event.h"
#include "CookingStation/Events/MouseEvent.h"
#include <memory>
#include <glm/glm.hpp>

class LevelCompletedPanel {
public:
    LevelCompletedPanel();
    void Init();
    void OnUpdate(float dt);
    void Draw(float screenW, float screenH, float baseScale);
    bool OnEvent(Event& e);
    void Show(int earnedMoney, int stars);
    bool IsOpen() const { return m_IsOpen; }

private:
    bool m_IsOpen = false;
    int m_EarnedMoney = 0;
    int m_Stars = 0;

    float m_DisplayMoney = 0.0f;
    float m_AnimationTimer = 0.0f;
    const float ANIMATION_DURATION = 1.5f; 

    glm::vec2 m_BtnPos = { 0.0f, 0.0f };
    glm::vec2 m_BtnSize = { 0.0f, 0.0f };

    std::shared_ptr<Texture> m_BgTexture;
    std::shared_ptr<Texture> m_LeftStarGrey, m_LeftStarYellow;
    std::shared_ptr<Texture> m_MiddleStarGrey, m_MiddleStarYellow;
    std::shared_ptr<Texture> m_RightStarGrey, m_RightStarYellow;
    std::shared_ptr<Texture> m_ButtonTexture;
};