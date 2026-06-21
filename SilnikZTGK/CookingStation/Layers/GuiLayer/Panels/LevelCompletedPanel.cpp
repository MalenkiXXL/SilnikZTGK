#include "LevelCompletedPanel.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Layers/GuiLayer/Utils/Renderer2D.h"
#include "CookingStation/Layers/GuiLayer/Utils/Gui.h"
#include "CookingStation/Core/Application.h"
#include "CookingStation/Events/GameEvents.h"
#include <spdlog/spdlog.h>

LevelCompletedPanel::LevelCompletedPanel() {}

void LevelCompletedPanel::Init() {
    m_BgTexture = AssetManager::GetTexture("assets://UI/LevelCompleted/dreamCompleted.png");
    m_ButtonTexture = AssetManager::GetTexture("assets://UI/LevelCompleted/backToMenuButton.png");

    m_LeftStarGrey = AssetManager::GetTexture("assets://UI/LevelCompleted/leftStarGrey.png");
    m_LeftStarYellow = AssetManager::GetTexture("assets://UI/LevelCompleted/leftStarYellow.png");

    m_MiddleStarGrey = AssetManager::GetTexture("assets://UI/LevelCompleted/middleStarGrey.png");
    m_MiddleStarYellow = AssetManager::GetTexture("assets://UI/LevelCompleted/middleStarYellow.png");

    m_RightStarGrey = AssetManager::GetTexture("assets://UI/LevelCompleted/rightStarGrey.png");
    m_RightStarYellow = AssetManager::GetTexture("assets://UI/LevelCompleted/rightStarYellow.png");
}

void LevelCompletedPanel::Show(int earnedMoney, int stars) {
    m_EarnedMoney = earnedMoney;
    m_Stars = stars;
    m_IsOpen = true;
    m_DisplayMoney = 0.0f;
    m_AnimationTimer = 0.0f;
}

void LevelCompletedPanel::OnUpdate(float dt) {
    if (!m_IsOpen) return;

    if (m_AnimationTimer < ANIMATION_DURATION) {
        m_AnimationTimer += dt;
        float progress = std::min(m_AnimationTimer / ANIMATION_DURATION, 1.0f);

        float easeOut = 1.0f - (1.0f - progress) * (1.0f - progress);

        m_DisplayMoney = easeOut * m_EarnedMoney;
    }
    else {
        m_DisplayMoney = (float)m_EarnedMoney;
    }
}

void LevelCompletedPanel::Draw(float screenW, float screenH, float baseScale) {
    if (!m_IsOpen || !m_BgTexture) return;

    float bgW = 700.0f * baseScale;
    float bgH = bgW * ((float)m_BgTexture->GetHeight() / (float)m_BgTexture->GetWidth());
    glm::vec2 bgPos = { (screenW - bgW) * 0.5f, (screenH - bgH) * 0.5f };

    // T³o (Dream Completed)
    Renderer2D::DrawQuad(bgPos, { bgW, bgH }, m_BgTexture, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });

    // --- UK£AD GWIAZDEK (£UK) ---
    float starW = 120.0f * baseScale;
    float starH = starW;
    float midStarW = starW * 1.25f;
    float midStarH = starH * 1.25f;

    float spacing = 20.0f * baseScale;
    float totalStarsW = starW + spacing + midStarW + spacing + starW;
    float startX = bgPos.x + (bgW - totalStarsW) * 0.5f;

    // Gwiazdki wy¿ej nad panelem
    float sideStarY = bgPos.y - 75.0f * baseScale;
    float midStarY = sideStarY - 35.0f * baseScale;

    auto texLeft = (m_Stars >= 1) ? m_LeftStarYellow : m_LeftStarGrey;
    auto texMiddle = (m_Stars >= 2) ? m_MiddleStarYellow : m_MiddleStarGrey;
    auto texRight = (m_Stars == 3) ? m_RightStarYellow : m_RightStarGrey;

    float leftX = startX;
    float midX = leftX + starW + spacing;
    float rightX = midX + midStarW + spacing;

    if (texLeft) Renderer2D::DrawQuad({ leftX, sideStarY }, { starW, starH }, texLeft, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    if (texMiddle) Renderer2D::DrawQuad({ midX, midStarY }, { midStarW, midStarH }, texMiddle, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    if (texRight) Renderer2D::DrawQuad({ rightX, sideStarY }, { starW, starH }, texRight, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });

    // --- ANIMOWANY TEKST PIENIÊDZY ---
    std::string moneyText = "$" + std::to_string((int)m_DisplayMoney);
    float textScale = 3.0f * baseScale;
    float textWidth = Gui::MeasureTextWidth(moneyText, textScale);

    glm::vec2 textPos = { bgPos.x + (bgW - textWidth) * 0.5f, bgPos.y + bgH * 0.40f };

    Gui::DrawGuiText(moneyText, { textPos.x + 4.0f, textPos.y + 4.0f }, textScale, { 0.0f, 0.0f, 0.0f, 0.5f });
    Gui::DrawGuiText(moneyText, textPos, textScale, { 1.0f, 0.85f, 0.1f, 1.0f });

    // --- PRZYCISK ---
    if (m_ButtonTexture) {
        float btnW = 320.0f * baseScale;
        float btnH = btnW * ((float)m_ButtonTexture->GetHeight() / (float)m_ButtonTexture->GetWidth());

        m_BtnPos = { bgPos.x + (bgW - btnW) * 0.5f, textPos.y + 180.0f * baseScale };
        m_BtnSize = { btnW, btnH };

        glm::vec2 mousePos = Gui::GetMappedMousePos();
        glm::vec4 tint = { 1.0f, 1.0f, 1.0f, 1.0f };
        if (mousePos.x >= m_BtnPos.x && mousePos.x <= m_BtnPos.x + m_BtnSize.x &&
            mousePos.y >= m_BtnPos.y && mousePos.y <= m_BtnPos.y + m_BtnSize.y) {
            tint = { 0.8f, 0.8f, 0.8f, 1.0f };
        }

        Renderer2D::DrawQuad(m_BtnPos, m_BtnSize, m_ButtonTexture, tint, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    }
}

bool LevelCompletedPanel::OnEvent(Event& e) {
    if (!m_IsOpen) return false;

    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& ev) {
        if (ev.GetButton() == 0) { 
            glm::vec2 mousePos = Gui::GetMappedMousePos();

            if (mousePos.x >= m_BtnPos.x && mousePos.x <= m_BtnPos.x + m_BtnSize.x &&
                mousePos.y >= m_BtnPos.y && mousePos.y <= m_BtnPos.y + m_BtnSize.y)
            {
                m_IsOpen = false;

                Application::Get().GetEventBus().Publish(ShowMainMenuEvent{});
                return true;
            }
        }
        return false;
        });

    if (e.GetEventType() == EventType::MouseButtonPressed ||
        e.GetEventType() == EventType::MouseScrolled ||
        e.GetEventType() == EventType::KeyPressed)
    {
        e.Handled = true;
    }

    return true;
}