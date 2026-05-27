#pragma once
#include "CookingStation/Events/MouseEvent.h"
#include "CookingStation/Layers/GuiLayer/Gui.h"
#include <glm/glm.hpp>
#include <algorithm>

class CarouselUI {
public:
    void Init(bool isLeftSided) {
        m_IsLeftSided = isLeftSided;
        m_ScrollIndex = 0;
        m_CurrentAngleOffset = 0.0f;
    }

    // NOWE: Wywo³ywane co klatkê, aby p³ynnie animowaæ przeskok
    void OnUpdate(float dt) {
        float targetOffset = m_ScrollIndex * m_AngleSpacing;
        // P³ynne doje¿d¿anie do celu (Lerp - szybkoœæ zale¿y od wartoœci 15.0f)
        m_CurrentAngleOffset += (targetOffset - m_CurrentAngleOffset) * 15.0f * dt;
    }

    void OnMouseScrolled(MouseScrolledEvent& e, float viewportWidth, int itemCount) {
        glm::vec2 mousePos = Gui::GetMappedMousePos();
        bool isMouseOnOurSide = m_IsLeftSided ? (mousePos.x < viewportWidth * 0.5f) : (mousePos.x >= viewportWidth * 0.5f);

        if (isMouseOnOurSide) {
            // ZMIANA: Zamiast p³ynnego k¹ta, zmieniamy docelowy INDEKS o dok³adnie 1
            if (e.GetYOffset() > 0.0f) {
                m_ScrollIndex--; // Scroll w górê
            }
            else if (e.GetYOffset() < 0.0f) {
                m_ScrollIndex++; // Scroll w dó³
            }

            // Zabezpieczenie przed wyjœciem w kosmos
            int maxIndex = std::max(0, itemCount - 1);
            if (m_ScrollIndex < 0) m_ScrollIndex = 0;
            if (m_ScrollIndex > maxIndex) m_ScrollIndex = maxIndex;
        }
    }

    // ZMIANA: arcRadius to teraz glm::vec2 (X i Y osobno!)
    bool GetItemTransform(int index, glm::vec2 centerPos, glm::vec2 arcRadius, glm::vec2 itemSize, glm::vec2& outPos) {
        float currentAngle = m_StartAngle - (index * m_AngleSpacing) + m_CurrentAngleOffset;

        if (currentAngle < 0.1f || currentAngle > 1.5f) {
            return false;
        }

        float xPos = m_IsLeftSided
            ? centerPos.x + arcRadius.x * cos(currentAngle)
            : centerPos.x - arcRadius.x * cos(currentAngle);

        float yPos = centerPos.y - arcRadius.y * sin(currentAngle);

        // U¿ywamy itemSize.x i itemSize.y, aby idealnie wyœrodkowaæ ka¿dy element, nawet te d³ugie!
        outPos = { xPos - (itemSize.x * 0.5f), yPos - (itemSize.y * 0.5f) };
        return true;
    }

private:
    int m_ScrollIndex = 0;           // Zawsze wskazuje pe³n¹ liczbê (0, 1, 2...)
    float m_CurrentAngleOffset = 0.0f; // Animowana wartoœæ d¹¿¹ca do celu
    bool m_IsLeftSided = true;

    float m_AngleSpacing = 0.55f;
    float m_StartAngle = 1.4f;
};