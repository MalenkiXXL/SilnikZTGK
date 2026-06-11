#pragma once
#include "CookingStation/Events/MouseEvent.h"
#include "Utils/Gui.h"
#include <glm/glm.hpp>
#include <algorithm>

class CarouselUI {
public:
    void Init(bool isLeftSided) {
        m_IsLeftSided = isLeftSided;
        m_ScrollIndex = 0;
        m_CurrentAngleOffset = 0.0f;
    }

    void OnUpdate(float dt) {
        float targetOffset = m_ScrollIndex * m_AngleSpacing;
        m_CurrentAngleOffset += (targetOffset - m_CurrentAngleOffset) * 15.0f * dt;
    }

    void OnMouseScrolled(MouseScrolledEvent& e, float viewportWidth, int itemCount) {
        glm::vec2 mousePos = Gui::GetMappedMousePos();
        bool isMouseOnOurSide = m_IsLeftSided ? (mousePos.x < viewportWidth * 0.5f) : (mousePos.x >= viewportWidth * 0.5f);

        if (isMouseOnOurSide) {
            if (e.GetYOffset() > 0.0f) {
                m_ScrollIndex--; 
            }
            else if (e.GetYOffset() < 0.0f) {
                m_ScrollIndex++; 
            }

            int maxIndex = std::max(0, itemCount - 1);
            if (m_ScrollIndex < 0) m_ScrollIndex = 0;
            if (m_ScrollIndex > maxIndex) m_ScrollIndex = maxIndex;
        }
    }

    bool GetItemTransform(int index, glm::vec2 centerPos, glm::vec2 arcRadius, glm::vec2 itemSize, glm::vec2& outPos) {
        float currentAngle = m_StartAngle - (index * m_AngleSpacing) + m_CurrentAngleOffset;

        if (currentAngle < 0.1f || currentAngle > 1.5f) {
            return false;
        }

        float xPos = m_IsLeftSided
            ? centerPos.x + arcRadius.x * cos(currentAngle)
            : centerPos.x - arcRadius.x * cos(currentAngle);

        float yPos = centerPos.y - arcRadius.y * sin(currentAngle);

        outPos = { xPos - (itemSize.x * 0.5f), yPos - (itemSize.y * 0.5f) };
        return true;
    }

private:
    int m_ScrollIndex = 0;           
    float m_CurrentAngleOffset = 0.0f; 
    bool m_IsLeftSided = true;

    float m_AngleSpacing = 0.55f;
    float m_StartAngle = 1.4f;
};