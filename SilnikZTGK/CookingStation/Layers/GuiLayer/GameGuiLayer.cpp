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
bool GameGuiLayer::s_NeedsQuestReload = false; // Inicjalizacja flagi

// === SYSTEM KOTWIC (VIRTUAL ANCHORING) ===

namespace { // Pocz¹tek anonimowej przestrzeni nazw
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
    // Inicjalizacja renderera 2D dla tej warstwy
    Renderer2D::Init();

    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = (float)windowSize.first;
    m_ViewportHeight = (float)windowSize.second;
    m_CornerIcon = AssetManager::GetTexture("CookingStation/Assets/UI/bottomCornerClouds.png");
}

void GameGuiLayer::OnUpdate(Timestep ts) {
    Gui::BeginFrame();
    Gui::UpdateDeltaTime(ts.GetSeconds());

    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    bool isPlayMode = (activeScene && activeScene->GetState() == SceneState::Play);

    // --- 1. WYMIARY EKRANU GRY (Skopiowane z EditorGuiLayer) ---
    float gameX = 200.0f;
    float gameY = 30.0f;
    float gameWidth = m_ViewportWidth - 500.0f;
    float gameHeight = m_ViewportHeight - 230.0f;

    // Zabezpieczenie przed b³êdem, gdy okno jest za ma³e
    if (gameWidth <= 0.0f || gameHeight <= 0.0f) return;

    glDisable(GL_DEPTH_TEST);
    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);
    Renderer2D::BeginScene(uiProj);

    // --- 2. W£¥CZENIE NO¯YC (SCISSOR TEST) ---
    // Zabezpiecza przed "wylewaniem" siê GUI na panele edytora
    Renderer2D::EndScene(); // Wypychamy ewentualne poprzednie rysowanie
    glEnable(GL_SCISSOR_TEST);
    // OpenGL liczy oœ Y od do³u ekranu, musimy to odwróciæ
    int scissorY = (int)(m_ViewportHeight - (gameY + gameHeight));
    glScissor((int)gameX, scissorY, (int)gameWidth, (int)gameHeight);
    Renderer2D::BeginScene(uiProj); // Zaczynamy now¹ paczkê (z no¿ycami)

    // --- 3. INTERAKTYWNY PANEL QUESTÓW ---
    if (!m_CurrentQuests.empty()) {
        glm::vec2 qpSize = { 380.0f, 200.0f };

        // ZMIANA: Zmniejszy³em offset Y ze 100 na 20, bo teraz liczymy od góry ekranu gry!
        glm::vec2 qpPos = GetAnchoredPosition(Anchor::TopRight, 20.0f, 20.0f, qpSize.x, qpSize.y, gameWidth, gameHeight);
        qpPos.x += gameX; // Przesuwamy w prawo o margines edytora
        qpPos.y += gameY; // Przesuwamy w dó³ o pasek u góry

        float alpha = isPlayMode ? 0.95f : 0.4f;
        Renderer2D::DrawQuad(qpPos, qpSize, { 0.12f, 0.12f, 0.12f, alpha }, 15.0f);

        const auto& q = m_CurrentQuests[m_CurrentQuestIndex];
        std::string header = "ZADANIE (" + std::to_string(m_CurrentQuestIndex + 1) + "/" + std::to_string(m_CurrentQuests.size()) + ")";
        Gui::DrawGuiText(header, { qpPos.x + 15.f, qpPos.y + 15.f }, 0.6f, { 1.0f, 0.5f, 0.2f, 1.0f });
        Gui::DrawGuiText(q.title, { qpPos.x + 15.f, qpPos.y + 45.f }, 0.55f, { 1.0f, 0.8f, 0.2f, 1.0f });

        std::string line1 = q.desc, line2 = "";
        if (line1.length() > 50) {
            size_t spacePos = line1.find_last_of(' ', 50);
            if (spacePos != std::string::npos) { line2 = line1.substr(spacePos + 1); line1 = line1.substr(0, spacePos); }
        }

        Gui::DrawGuiText(line1, { qpPos.x + 15.f, qpPos.y + 75.f }, 0.45f, { 0.9f, 0.9f, 0.9f, 1.0f });
        if (!line2.empty()) Gui::DrawGuiText(line2, { qpPos.x + 15.f, qpPos.y + 95.f }, 0.45f, { 0.9f, 0.9f, 0.9f, 1.0f });

        Gui::DrawGuiText("Cel: " + std::to_string(q.portions) + " porcji", { qpPos.x + 15.f, qpPos.y + 145.f }, 0.5f, { 0.5f, 1.0f, 0.5f, 1.0f });
        Gui::DrawGuiText("Nagroda: " + q.reward, { qpPos.x + 15.f, qpPos.y + 170.f }, 0.45f, { 0.4f, 0.8f, 1.0f, 1.0f });

        if (isPlayMode) {
            if (Gui::Button("< Poprzednie", { qpPos.x, qpPos.y + qpSize.y + 5.f }, { 130.f, 30.f })) {
                if (m_CurrentQuestIndex > 0) m_CurrentQuestIndex--;
            }
            if (Gui::Button("Nastepne >", { qpPos.x + qpSize.x - 130.f, qpPos.y + qpSize.y + 5.f }, { 130.f, 30.f })) {
                if (m_CurrentQuestIndex < (int)m_CurrentQuests.size() - 1) m_CurrentQuestIndex++;
            }
        }
    }

    // Chmurki w rogach ekranu - UI do sk³adników i maszyn
    if (m_CornerIcon) {
        float iconH = 300.0f;
        float iconW = iconH * 1239.0f / 1024.0f;

        glm::vec2 iconSize = { iconW, iconH };

        float padding = 0.0f;

        glm::vec2 leftPos = {
            gameX + padding,
            gameY + gameHeight - iconSize.y - padding
        };

        glm::vec2 rightPos = {
            gameX + gameWidth - iconSize.x - padding,
            gameY + gameHeight - iconSize.y - padding
        };

        Renderer2D::DrawQuad(
            leftPos,
            iconSize,
            m_CornerIcon,
            { 1.0f, 1.0f, 1.0f, 1.0f },
            { 0.0f, 1.0f },
            { 1.0f, 0.0f }
        );

        Renderer2D::DrawQuad(
            rightPos,
            iconSize,
            m_CornerIcon,
            { 1.0f, 1.0f, 1.0f, 1.0f },
            { 1.0f, 1.0f },
            { 0.0f, 0.0f }
        );
    }

    // --- 4. WY£¥CZENIE NO¯YC ---
    Renderer2D::EndScene();
    glDisable(GL_SCISSOR_TEST); // Oddajemy pe³ny ekran z powrotem!
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

    // Przechwytywanie klikniêæ, aby nie "strzelaæ" przez UI gry
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
    if (!activeScene || activeScene->GetState() != SceneState::Play) {
        return false;
    }

    auto mousePos = Input::GetMousePosition();
    float mouseX = mousePos.first;
    float mouseY = mousePos.second;

    auto IsInside = [&](float x, float y, float w, float h) {
        return mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h;
        };

    // Identyczne wyliczenia co w OnUpdate
    float gameX = 200.0f;
    float gameY = 30.0f;
    float gameWidth = m_ViewportWidth - 500.0f;
    float gameHeight = m_ViewportHeight - 230.0f;

    if (!m_CurrentQuests.empty()) {
        glm::vec2 size(380.0f, 200.0f);

        // Wyliczamy pozycjê bazuj¹c na ekranie gry
        glm::vec2 pos = GetAnchoredPosition(Anchor::TopRight, 20.0f, 20.0f, size.x, size.y, gameWidth, gameHeight);
        pos.x += gameX;
        pos.y += gameY;

        if (IsInside(pos.x, pos.y, size.x, size.y + 40.0f)) {
            return true; // Klikniêcie zosta³o uwiêzione przez panel UI Gry
        }
    }

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
            spdlog::info("GameUiLayer: Questy zaladowane do interfejsu gry.");
        }
        catch (...) {
            spdlog::error("GameUiLayer: Blad parsowania JSONa questow.");
        }
    }
}