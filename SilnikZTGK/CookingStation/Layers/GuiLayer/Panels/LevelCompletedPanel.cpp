#include "LevelCompletedPanel.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Layers/GuiLayer/Utils/Renderer2D.h"
#include "CookingStation/Layers/GuiLayer/Utils/Gui.h"
#include "CookingStation/Core/Application.h"
#include "CookingStation/Events/GameEvents.h"
#include <spdlog/spdlog.h>
#include <cmath>     
#include <algorithm> 

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

    m_AnimationTimer += dt;

    if (m_AnimationTimer < ANIMATION_DURATION) {
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

    Renderer2D::DrawQuad(bgPos, { bgW, bgH }, m_BgTexture, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });

    float baseStarW = 120.0f * baseScale;
    float baseStarH = baseStarW;
    float baseMidStarW = baseStarW * 1.25f;
    float baseMidStarH = baseStarH * 1.25f;

    float spacing = 20.0f * baseScale;
    float totalStarsW = baseStarW + spacing + baseMidStarW + spacing + baseStarW;
    float startX = bgPos.x + (bgW - totalStarsW) * 0.5f;

    // Gwiazdki wy ej nad panelem
    float sideStarY = bgPos.y - 75.0f * baseScale;
    float midStarY = sideStarY - 35.0f * baseScale;

    float leftX = startX;
    float midX = leftX + baseStarW + spacing;
    float rightX = midX + baseMidStarW + spacing;

    float trigger1 = 0.3f; // Kiedy zapala si  1 gwiazdka (Lewa)
    float trigger2 = 0.7f; // Kiedy zapala si  2 gwiazdka (Prawa)
    float trigger3 = 1.1f; // Kiedy zapala si  3 gwiazdka ( rodkowa)
    float popDuration = 0.4f; // Jak d ugo trwa powi kszenie

    // Funkcja wyliczaj ca p ynn  skal  (powi ksza o 40% w szczycie sinusa)
    auto getScale = [&](float trigger) {
        if (m_AnimationTimer >= trigger && m_AnimationTimer < trigger + popDuration) {
            float t = (m_AnimationTimer - trigger) / popDuration; // Zmienna od 0.0 do 1.0
            return 1.0f + 0.4f * std::sin(t * 3.14159265f); // 3.14 to po owa cyklu sinusa
        }
        return 1.0f; // Domy lna skala przed i po animacji
        };

    // ZMIANA: Przypisanie czas w i wymaganej liczby gwiazdek do nowej kolejno ci
    float scaleLeft = (m_Stars >= 1) ? getScale(trigger1) : 1.0f;
    float scaleRight = (m_Stars >= 2) ? getScale(trigger2) : 1.0f; // Prawa zapala si  jako druga
    float scaleMid = (m_Stars == 3) ? getScale(trigger3) : 1.0f; //  rodkowa zapala si  jako trzecia

    // ZMIANA: Wybieramy z ot  tekstur  zgodnie z now  kolejno ci 
    auto texLeft = (m_Stars >= 1 && m_AnimationTimer >= trigger1) ? m_LeftStarYellow : m_LeftStarGrey;
    auto texRight = (m_Stars >= 2 && m_AnimationTimer >= trigger2) ? m_RightStarYellow : m_RightStarGrey;
    auto texMiddle = (m_Stars == 3 && m_AnimationTimer >= trigger3) ? m_MiddleStarYellow : m_MiddleStarGrey;

    // Funkcja renderuj ca z zachowaniem wy rodkowania
    auto drawStar = [&](std::shared_ptr<Texture> tex, float x, float y, float w, float h, float scale) {
        if (!tex) return;
        float finalW = w * scale;
        float finalH = h * scale;
        float finalX = x - (finalW - w) * 0.5f;
        float finalY = y - (finalH - h) * 0.5f;
        Renderer2D::DrawQuad({ finalX, finalY }, { finalW, finalH }, tex, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
        };

    // Rysowanie poszczeg lnych gwiazdek (tutaj kolejno   wywo a  nie ma znaczenia, licz  si  parametry wy ej)
    drawStar(texLeft, leftX, sideStarY, baseStarW, baseStarH, scaleLeft);
    drawStar(texMiddle, midX, midStarY, baseMidStarW, baseMidStarH, scaleMid);
    drawStar(texRight, rightX, sideStarY, baseStarW, baseStarH, scaleRight);

    // --- ANIMOWANY TEKST PIENI DZY ---
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