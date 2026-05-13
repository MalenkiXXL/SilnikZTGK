#include "GameGuiLayer.h"
#include "EditorGuiLayer.h"
#include "Gui.h"
#include "Renderer2D.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Events/EditorEvents.h" 
#include "CookingStation/Core/Application.h"
#include <fstream>
#include "CookingStation/json.hpp"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include <algorithm> // dla std::max
#include "CookingStation/Scripts/DragAndDropScript.h"

bool GameGuiLayer::s_NeedsQuestReload = false;

namespace {
    enum class Anchor { TopLeft, TopRight, BottomLeft, BottomRight, Center };

    glm::vec2 GetAnchoredPosition(Anchor anchor, float offsetX, float offsetY, float width, float height, float screenWidth, float screenHeight) {
        switch (anchor) {
        case Anchor::TopLeft:     return { offsetX, offsetY };
        case Anchor::TopRight:    return { screenWidth - width - offsetX, offsetY };
        case Anchor::BottomLeft:  return { offsetX, screenHeight - height - offsetY };
        case Anchor::BottomRight: return { screenWidth - width - offsetX, screenHeight - height - offsetY };
        case Anchor::Center:      return { (screenWidth - width) / 2.0f + offsetX, (screenHeight - height) / 2.0f + offsetY };
        }
        return { offsetX, offsetY };
    }
}

void GameGuiLayer::OnAttach() {
    Renderer2D::Init();
    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = (float)windowSize.first;
    m_ViewportHeight = (float)windowSize.second;
    m_CornerIcon = AssetManager::GetTexture("CookingStation/Assets/UI/bottomCornerClouds.png");
    m_TomatoIcon = AssetManager::GetTexture("CookingStation/Assets/UI/tomato.png");
}


bool GameGuiLayer::DrawBubblyImage(const std::string& id, std::shared_ptr<Texture> icon, glm::vec2 basePos, glm::vec2 baseSize, float dt, float hoverScale, bool darkenOnHover)
{
    if (!icon) return false;

    auto& state = m_BubblyStates[id];
    auto mousePos = Input::GetMousePosition();
    float animSpeed = 15.0f;

    bool isHovered = (mousePos.first >= basePos.x && mousePos.first <= basePos.x + baseSize.x &&
        mousePos.second >= basePos.y && mousePos.second <= basePos.y + baseSize.y);

    float targetScale = isHovered ? hoverScale : 1.0f;
    glm::vec4 targetColor = (isHovered && darkenOnHover) ? glm::vec4(0.8f, 0.8f, 0.8f, 1.0f) : glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    state.scale += (targetScale - state.scale) * dt * animSpeed;
    state.color.r += (targetColor.r - state.color.r) * dt * animSpeed;
    state.color.g += (targetColor.g - state.color.g) * dt * animSpeed;
    state.color.b += (targetColor.b - state.color.b) * dt * animSpeed;

    glm::vec2 size = baseSize * state.scale;
    glm::vec2 pos = {
        basePos.x + (baseSize.x * 0.5f) - (size.x * 0.5f),
        basePos.y + (baseSize.y * 0.5f) - (size.y * 0.5f)
    };

    if (id == "CloudRight") {
        Renderer2D::DrawQuad(pos, size, icon, state.color, { 1.0f, 1.0f }, { 0.0f, 0.0f });
    }
    else {
        Renderer2D::DrawQuad(pos, size, icon, state.color, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    }

    return (Input::IsMouseButtonJustPressed(0) && isHovered);
}

void GameGuiLayer::OnUpdate(Timestep ts) {
    Gui::BeginFrame();
    Gui::UpdateDeltaTime(ts.GetSeconds());

    if (s_NeedsQuestReload) {
        ReloadQuests();
        s_NeedsQuestReload = false;
    }

    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    bool isPlayMode = (activeScene && activeScene->GetState() == SceneState::Play);

    // --- 1. WYMIARY OBSZARU GRY ---
    float gameX = 200.0f;
    float gameY = 30.0f;
    float gameWidth = m_ViewportWidth - 500.0f;
    float gameHeight = m_ViewportHeight - 230.0f;

    if (gameWidth <= 0.0f || gameHeight <= 0.0f) return;

    // --- 2. DYNAMICZNA SKALA (Responsywnoœæ) ---
    // Obliczamy skalê wzglêdem wysokoœci 1080p, z progiem bezpieczeñstwa 0.5
    float baseScale = std::max(gameHeight / 1080.0f, 0.5f);

    glDisable(GL_DEPTH_TEST);
    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);
    Renderer2D::BeginScene(uiProj);

    // --- 3. SCISSOR TEST ---
    Renderer2D::EndScene();
    glEnable(GL_SCISSOR_TEST);
    int scissorY = (int)(m_ViewportHeight - (gameY + gameHeight));
    glScissor((int)gameX, scissorY, (int)gameWidth, (int)gameHeight);
    Renderer2D::BeginScene(uiProj);

    // --- 4. PANEL QUESTÓW (SKALOWALNY) ---
    if (!m_CurrentQuests.empty()) {
        glm::vec2 qpSize = { 380.0f * baseScale, 220.0f * baseScale }; // Skalujemy rozmiar panelu
        float margin = 20.0f * baseScale; // Skalujemy margines

        glm::vec2 qpPos = GetAnchoredPosition(Anchor::TopRight, margin, margin, qpSize.x, qpSize.y, gameWidth, gameHeight);
        qpPos.x += gameX;
        qpPos.y += gameY;

        float alpha = isPlayMode ? 0.95f : 0.4f;
        Gui::Panel(qpPos, qpSize, { 0.12f, 0.12f, 0.12f, alpha }, 15.0f * baseScale);

        const auto& q = m_CurrentQuests[m_CurrentQuestIndex];

        // Skalujemy pozycje tekstu wewn¹trz panelu oraz skalê czcionki
        float textX = qpPos.x + 15.f * baseScale;
        std::string header = "ZADANIE (" + std::to_string(m_CurrentQuestIndex + 1) + "/" + std::to_string(m_CurrentQuests.size()) + ")";
        Gui::DrawGuiText(header, { textX, qpPos.y + 15.f * baseScale }, 0.6f * baseScale, { 1.0f, 0.5f, 0.2f, 1.0f });
        Gui::DrawGuiText(q.title, { textX, qpPos.y + 50.f * baseScale }, 0.55f * baseScale, { 1.0f, 0.8f, 0.2f, 1.0f });

        // Opis z prostym zawijaniem
        std::string line1 = q.desc, line2 = "";
        if (line1.length() > 45) {
            size_t spacePos = line1.find_last_of(' ', 45);
            if (spacePos != std::string::npos) { line2 = line1.substr(spacePos + 1); line1 = line1.substr(0, spacePos); }
        }

        Gui::DrawGuiText(line1, { textX, qpPos.y + 85.f * baseScale }, 0.45f * baseScale, { 0.9f, 0.9f, 0.9f, 1.0f });
        if (!line2.empty()) Gui::DrawGuiText(line2, { textX, qpPos.y + 110.f * baseScale }, 0.45f * baseScale, { 0.9f, 0.9f, 0.9f, 1.0f });

        Gui::DrawGuiText("Cel: " + std::to_string(q.portions), { textX, qpPos.y + 155.f * baseScale }, 0.5f * baseScale, { 0.5f, 1.0f, 0.5f, 1.0f });
        Gui::DrawGuiText("Nagroda: " + q.reward, { textX, qpPos.y + 185.f * baseScale }, 0.45f * baseScale, { 0.4f, 0.8f, 1.0f, 1.0f });

        if (isPlayMode) {
            glm::vec2 btnSize = { 130.f * baseScale, 35.f * baseScale };
            if (Gui::Button("< Poprz.", { qpPos.x, qpPos.y + qpSize.y + 5.f * baseScale }, btnSize)) {
                if (m_CurrentQuestIndex > 0) m_CurrentQuestIndex--;
            }
            if (Gui::Button("Nast. >", { qpPos.x + qpSize.x - btnSize.x, qpPos.y + qpSize.y + 5.f * baseScale }, btnSize)) {
                if (m_CurrentQuestIndex < (int)m_CurrentQuests.size() - 1) m_CurrentQuestIndex++;
            }
        }
    }

    // --- 5. CHMURY I SK£ADNIKI ---
    if (m_CornerIcon) {
        float iconH = gameHeight * 0.30f;
        float iconW = iconH * (1239.0f / 1024.0f);
        glm::vec2 baseIconSize = { iconW, iconH };

        glm::vec2 leftPosBase = { gameX, gameY + gameHeight - baseIconSize.y };
        glm::vec2 rightPosBase = { gameX + gameWidth - baseIconSize.x, gameY + gameHeight - baseIconSize.y };

        float dt = ts.GetSeconds();

        // Rusyjemy chmury
        DrawBubblyImage("CloudLeft", m_CornerIcon, leftPosBase, baseIconSize, dt, 1.15f, false);
        DrawBubblyImage("CloudRight", m_CornerIcon, rightPosBase, baseIconSize, dt, 1.15f, false);

        // Rysujemy sk³adniki na lewej chmurze
        if (m_TomatoIcon) {
            float tomatoBaseH = baseIconSize.y * 0.3f;
            glm::vec2 tomatoBaseSize = { tomatoBaseH, tomatoBaseH };
            glm::vec2 tomatoBasePos = {
                leftPosBase.x + (baseIconSize.x * 0.5f) - (tomatoBaseSize.x * 0.5f),
                leftPosBase.y + (baseIconSize.y * 0.5f) - (tomatoBaseSize.y * 0.5f)
            };
            if (DrawBubblyImage("BtnTomato", m_TomatoIcon, tomatoBasePos, tomatoBaseSize, dt, 1.30f, true)) {
                spdlog::info("UI: Wyci¹gniêto pomidora!");
                DragAndDropScript::StartDrag(IngredientType::Tomato, "CookingStation/Assets/models/skladniki/pomidor/pomidor.gltf");
            }
        }
    }

    Renderer2D::EndScene();
    glDisable(GL_SCISSOR_TEST);
    Renderer2D::BeginScene(uiProj);
    Renderer2D::EndScene();
    glEnable(GL_DEPTH_TEST);
}

void GameGuiLayer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
        m_ViewportWidth = (float)ev.GetWidth();
        m_ViewportHeight = (float)ev.GetHeight();
        return false;
        });

    dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& ev) {
        return OnMouseButtonPressed(ev);
        });

    dispatcher.Dispatch<ScenePlayEvent>([this](ScenePlayEvent& ev) {
        ReloadQuests();
        return false;
        });
}

bool GameGuiLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e) {
    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    if (!activeScene || activeScene->GetState() != SceneState::Play) return false;

    // Przechwytywanie myszy przez skalowalne panele/przyciski
    if (Gui::WantCaptureMouse()) return true;

    return false;
}

void GameGuiLayer::ReloadQuests() {
    m_CurrentQuests.clear();
    std::ifstream file("CookingStation/Assets/wygenerowane_quests.json");
    if (file.is_open()) {
        try {
            nlohmann::json data = nlohmann::json::parse(file);
            for (auto& q : data) {
                m_CurrentQuests.push_back({
                    q.value("title", "Brak tytulu"),
                    q.value("description", "Brak opisu"),
                    q.value("portions", 0),
                    q.value("reward", "Brak nagrody")
                    });
            }
            m_CurrentQuestIndex = 0;
            spdlog::info("GameUiLayer: Questy zaladowane responsywnie.");
        }
        catch (...) {
            spdlog::error("GameUiLayer: Blad JSON.");
        }
    }
}