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
    m_BookCloudIcon = AssetManager::GetTexture("CookingStation/Assets/UI/bookCloud.png");
    m_BookIcon = AssetManager::GetTexture("CookingStation/Assets/UI/book.png");
    m_BookStarsIcon = AssetManager::GetTexture("CookingStation/Assets/UI/bookStars.png");
    m_BookInsideIcon = AssetManager::GetTexture("CookingStation/Assets/UI/bookInside.png");
    m_BookXIcon = AssetManager::GetTexture("CookingStation/Assets/UI/bookX.png");
}


bool GameGuiLayer::DrawBubblyImage(const std::string& id, std::shared_ptr<Texture> icon, glm::vec2 basePos, glm::vec2 baseSize, float dt, float hoverScale, bool darkenOnHover, float hitRadiusMultiplier)
{
    if (!icon) return false;

    auto& state = m_BubblyStates[id];
    auto mousePos = Input::GetMousePosition();
    float animSpeed = 15.0f;

    glm::vec2 center = { basePos.x + baseSize.x * 0.5f, basePos.y + baseSize.y * 0.5f };

    float hitRadius = std::min(baseSize.x, baseSize.y) * hitRadiusMultiplier;

    float distX = mousePos.first - center.x;
    float distY = mousePos.second - center.y;

    bool isHovered = (distX * distX + distY * distY) <= (hitRadius * hitRadius);

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
    float dt = ts.GetSeconds();

    if (s_NeedsQuestReload) {
        ReloadQuests();
        s_NeedsQuestReload = false;
    }

    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    bool isPlayMode = (activeScene && activeScene->GetState() == SceneState::Play);

    // --- WYMIARY OBSZARU GRY ---
    float gameX = 200.0f;
    float gameY = 30.0f;
    float gameWidth = m_ViewportWidth - 500.0f;
    float gameHeight = m_ViewportHeight - 230.0f;

    if (gameWidth <= 0.0f || gameHeight <= 0.0f) return;

    // --- DYNAMICZNA SKALA ---
    // Obliczamy skalê wzglêdem wysokoœci 1080p, z progiem bezpieczeñstwa 0.5
    float baseScale = std::max(gameHeight / 1080.0f, 0.5f);

    glDisable(GL_DEPTH_TEST);
    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);
    Renderer2D::BeginScene(uiProj);

    // --- SCISSOR TEST ---
    Renderer2D::EndScene();
    glEnable(GL_SCISSOR_TEST);
    int scissorY = (int)(m_ViewportHeight - (gameY + gameHeight));
    glScissor((int)gameX, scissorY, (int)gameWidth, (int)gameHeight);
    Renderer2D::BeginScene(uiProj);

    // --- PANEL QUESTÓW ---
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

    // --- CHMURY I SK£ADNIKI ---
    if (m_CornerIcon) {
        float iconH = gameHeight * 0.30f;
        float iconW = iconH * (1239.0f / 1024.0f);
        glm::vec2 baseIconSize = { iconW, iconH };

        glm::vec2 leftPosBase = { gameX, gameY + gameHeight - baseIconSize.y };
        glm::vec2 rightPosBase = { gameX + gameWidth - baseIconSize.x, gameY + gameHeight - baseIconSize.y };

        // Rusyjemy chmury
        DrawBubblyImage("CloudLeft", m_CornerIcon, leftPosBase, baseIconSize, dt, 1.15f, false, 0.55f);
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


    // --- KSI¥¯KA Z PRZEPISAMI ---
    if (m_BookIcon) {
        glm::vec2 cloudSize = { 280.0f * baseScale, 280.0f * baseScale };
        glm::vec2 cloudPos = { gameX + 10.0f * baseScale, gameY + 2.0f * baseScale };

        if (!m_IsRecipeBookOpen) {
            // Rozmiar chmury
            glm::vec2 actualCloudSize = cloudSize * 1.3f;

            // Chmurka w tle 
            if (m_BookCloudIcon) DrawBubblyImage("BookCloud", m_BookCloudIcon, cloudPos, actualCloudSize, dt, 1.1f, false, 0.55f);

            // Ksi¹¿ka
            glm::vec2 bookSize = cloudSize * 1.1f;
            glm::vec2 bookPos = {
                cloudPos.x + (actualCloudSize.x - bookSize.x) * 0.5f,
                cloudPos.y + (actualCloudSize.y - bookSize.y) * 0.5f
            };

            if (DrawBubblyImage("BookIcon", m_BookIcon, bookPos, bookSize, dt, 1.2f, true, 0.25f)) {
                m_IsRecipeBookOpen = true;
                spdlog::info("UI: Otwarto ksiazke z przepisami!");
            }

            // Gwiazdki na wierzchu 
            if (m_BookStarsIcon) DrawBubblyImage("BookStars", m_BookStarsIcon, cloudPos, actualCloudSize, dt, 1.15f, false, 0.55f);
        }
        else {
            // =========================================================
            // --- 1. WNÊTRZE KSI¥¯KI (NAPRAWA SP£ASZCZENIA) ---
            // =========================================================

            // Zamiast rozci¹gaæ szerokoœæ na 0.7f z gameWidth (co sp³aszcza³o obraz),
            // robimy na twardo 85% wysokoœci ekranu i wyliczamy szerokoœæ z proporcji pliku!
            float rawWidth = (float)m_BookInsideIcon->GetWidth();
            float rawHeight = (float)m_BookInsideIcon->GetHeight();
            float aspect = rawWidth / rawHeight;

            glm::vec2 insideSize;
            insideSize.y = gameHeight * 1.0f; // Twarda wysokoœæ na œrodku
            insideSize.x = insideSize.y * aspect; // Wyliczona, niesp³aszczona szerokoœæ!

            float yOffset = 50.0f * baseScale;

            // Centrujemy niesp³aszczone wnêtrze ksi¹¿ki idealnie na œrodku
            glm::vec2 insidePos = {
                gameX + (gameWidth - insideSize.x) * 0.5f,
                gameY + (gameHeight - insideSize.y) * 0.5f + yOffset
            };

            if (m_BookInsideIcon) {
                // T³o ksi¹¿ki (wyg³adzanie hover=false, skala=1.0)
                DrawBubblyImage("BookInside", m_BookInsideIcon, insidePos, insideSize, dt, 1.0f, false);
            }

            // =========================================================
            // --- 2. X DO ZAMYKANIA (NAPRAWA MA£EJ WIELKOŒCI I POZYCJI) ---
            // =========================================================

            // 2.1 Powiekszamy X: Zmieni³em z 60.0f na 110.0f, ¿eby by³ du¿y i czytelny (jak na Twoim screenshocie).
            glm::vec2 xSize = { 400.0f * baseScale, 400.0f * baseScale };

            // 2.2 Naprawa pozycji: Poniewa¿ szerokoœæ ksi¹¿ki nie jest ju¿ rozci¹gniêta, standardowe pozycjonowanie dzia³a idealnie.
            // Odsuwamy X lekko od prawej krawêdzi i góry, ¿eby ³adnie le¿a³ wewn¹trz ksi¹¿ki.
            glm::vec2 xPos = {
                insidePos.x + insideSize.x - xSize.x * 1.5f,
                insidePos.y + xSize.y * 0.5f
            };

            if (m_BookXIcon) {
                // Klikniêcie w X zamyka panel, reaguje na najechanie z ciemnieniem (true) i ma mniejszy kolizyjny promieñ (0.4f)
                if (DrawBubblyImage("BookX", m_BookXIcon, xPos, xSize, dt, 1.2f, true, 0.4f)) {
                    m_IsRecipeBookOpen = false;
                    spdlog::info("UI: Zamknieto ksiazke z przepisami!");
                }
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