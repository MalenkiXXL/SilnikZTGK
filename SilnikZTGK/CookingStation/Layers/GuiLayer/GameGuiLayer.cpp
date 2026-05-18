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
#include <algorithm> 
#include "CookingStation/Scripts/DragAndDropScript.h"
#include "CookingStation/Core/GameProgress.h"

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
    m_CornerIcon = AssetManager::GetTexture("assets://UI/bottomCornerClouds.png");
    m_TomatoIcon = AssetManager::GetTexture("assets://UI/tomato.png");
    m_BookCloudIcon = AssetManager::GetTexture("assets://UI/bookCloud.png");
    m_BookIcon = AssetManager::GetTexture("assets://UI/book.png");
    m_BookStarsIcon = AssetManager::GetTexture("assets://UI/bookStars.png");
    m_BookInsideIcon = AssetManager::GetTexture("assets://UI/bookInside.png");
    m_BookXIcon = AssetManager::GetTexture("assets://UI/bookX.png");
    m_TomatoSoupIcon = AssetManager::GetTexture("assets://UI/tomatoSoup.png");

}

// Funkcja pomocnicza do rysowania "bubbly" ikon z efektem powi�kszenia i opcjonalnego przyciemnienia na hoverze
bool GameGuiLayer::DrawBubblyImage(const std::string& id, const std::shared_ptr<Texture>& icon, glm::vec2 basePos, glm::vec2 baseSize, float dt, float hoverScale, bool darkenOnHover, float hitRadiusMultiplier, glm::vec4 tintColor, bool* outIsHovered)
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

    if (outIsHovered != nullptr) {
        *outIsHovered = isHovered;
    }

    float targetScale = isHovered ? hoverScale : 1.0f;

    // Kolor tintu potrzebny do wyciemnienia w ksi��ce kucharskiej
    glm::vec4 targetColor = (isHovered && darkenOnHover) ? tintColor * glm::vec4(0.8f, 0.8f, 0.8f, 1.0f) : tintColor;

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

bool GameGuiLayer::DrawIngredientIcon(const std::string& id, const std::shared_ptr<Texture>& icon, glm::vec2 basePos, glm::vec2 baseSize, float dt, float baseScale, int count, bool showCount)
{
    bool isHovered = false;

    // 1. Odpalamy rysowanie ikony i zapisujemy, czy myszka na nią najechała do zmiennej isHovered
    bool isClicked = DrawBubblyImage(id, icon, basePos, baseSize, dt, 1.30f, true, 0.5f, { 1.0f, 1.0f, 1.0f, 1.0f }, &isHovered);

    // 2. Jeśli flaga showCount jest true ORAZ myszka jest na ikonie -> rysujemy tekst
    if (showCount && isHovered) {
        DrawIngredientCountText(count, basePos, baseSize, baseScale);
    }

    return isClicked;
}

void GameGuiLayer::DrawIngredientCountText(int count, glm::vec2 basePos, glm::vec2 baseSize, float baseScale)
{
    std::string countText = "x" + std::to_string(count);
    float textScale = 1.2f * baseScale;

    // Pozycja w lewym górnym rogu ikony
    glm::vec2 textPos = {
            basePos.x + (baseSize.x * 0.05f),
            basePos.y + (baseSize.y * 0.25f)
    };

    // Cień pod tekstem
    glm::vec2 shadowPos = { textPos.x + 3.0f, textPos.y + 3.0f };
    Gui::DrawGuiText(countText, shadowPos, textScale, { 0.1f, 0.1f, 0.1f, 0.9f });

    // Właściwy tekst na wierzchu
    Gui::DrawGuiText(countText, textPos, textScale, { 1.0f, 0.95f, 0.9f, 1.0f });
}

// Funkcja pomocnicza do obliczania rozmiaru ikony z zachowaniem proporcji
glm::vec2 GameGuiLayer::CalculateAspectSize(const std::shared_ptr<Texture>& texture, float targetHeight) {
    if (!texture) return { targetHeight, targetHeight }; // Fallback
    float aspect = (float)texture->GetWidth() / (float)texture->GetHeight();
    return { targetHeight * aspect, targetHeight };
}

// Funkcja do rysowania ikony przepisu w ksi��ce kucharskiej, z obs�ug� odblokowania 
void GameGuiLayer::DrawRecipeIcon(const std::string& recipeId, const std::shared_ptr<Texture>& texture,
    glm::vec2 relativePct, float targetHeight,
    glm::vec2 bookPos, glm::vec2 bookSize, float dt)
{
    if (!texture) return;

    glm::vec2 size = CalculateAspectSize(texture, targetHeight);

    glm::vec2 pos = {
        bookPos.x + bookSize.x * relativePct.x,
        bookPos.y + bookSize.y * relativePct.y
    };


    bool isUnlocked = GameProgress::IsRecipeUnlocked(recipeId);
    glm::vec4 tint = isUnlocked ? glm::vec4(1.0f) : glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);

    DrawBubblyImage("Recipe_" + recipeId, texture, pos, size, dt, 1.15f, true, 0.5f, tint);
}

void GameGuiLayer::DrawQuestPanel(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, bool isPlayMode) {
    if (!m_CurrentQuests.empty()) {
        glm::vec2 qpSize = { 380.0f * baseScale, 220.0f * baseScale }; // Skalujemy rozmiar panelu
        float margin = 20.0f * baseScale; // Skalujemy margines

        glm::vec2 qpPos = GetAnchoredPosition(Anchor::TopRight, margin, margin, qpSize.x, qpSize.y, gameWidth, gameHeight);
        qpPos.x += gameX;
        qpPos.y += gameY;

        float alpha = isPlayMode ? 0.95f : 0.4f;
        Gui::Panel(qpPos, qpSize, { 0.12f, 0.12f, 0.12f, alpha }, 15.0f * baseScale);

        const auto& q = m_CurrentQuests[m_CurrentQuestIndex];

        // Skalujemy pozycje tekstu wewn�trz panelu oraz skal� czcionki
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
}

void GameGuiLayer::DrawIngredientClouds(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt) {
    if (m_CornerIcon) {
        glm::vec2 baseIconSize = CalculateAspectSize(m_CornerIcon, gameHeight * 0.30f);

        glm::vec2 leftPosBase = { gameX, gameY + gameHeight - baseIconSize.y };
        glm::vec2 rightPosBase = { gameX + gameWidth - baseIconSize.x, gameY + gameHeight - baseIconSize.y };

        // Rusyjemy chmury
        DrawBubblyImage("CloudLeft", m_CornerIcon, leftPosBase, baseIconSize, dt, 1.15f, false, 0.55f);
        DrawBubblyImage("CloudRight", m_CornerIcon, rightPosBase, baseIconSize, dt, 1.15f, false);

        // Rysujemy sk�adniki na lewej chmurze
        if (m_TomatoIcon) {
            float tomatoBaseH = baseIconSize.y * 0.3f;
            glm::vec2 tomatoBaseSize = { tomatoBaseH, tomatoBaseH };
            glm::vec2 tomatoBasePos = {
                leftPosBase.x + (baseIconSize.x * 0.5f) - (tomatoBaseSize.x * 0.5f),
                leftPosBase.y + (baseIconSize.y * 0.5f) - (tomatoBaseSize.y * 0.5f)
            };

            int tomatoCount = GameManagerScript::s_Instance ? GameManagerScript::s_Instance->GetIngredientCount(IngredientType::Tomato) : 0;
            bool showCountText = true;

            if (DrawIngredientIcon("BtnTomato", m_TomatoIcon, tomatoBasePos, tomatoBaseSize, dt, baseScale, tomatoCount, showCountText)) {
                spdlog::info("UI: Wyci�gni�to pomidora!");
                DragAndDropScript::StartDrag(IngredientType::Tomato, "CookingStation/Assets/models/skladniki/pomidor/pomidor.gltf");
            }
        }
    }
}

void GameGuiLayer::DrawRecipeBook(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt) {
    // --- KSI��KA Z PRZEPISAMI ---
    if (m_BookIcon) {
        glm::vec2 cloudSize = { 280.0f * baseScale, 280.0f * baseScale };
        glm::vec2 cloudPos = { gameX + 20.0f * baseScale, gameY + 20.0f * baseScale };
        glm::vec2 actualCloudSize = cloudSize * 1.3f;

        // 1. CHMURKA
        if (m_BookCloudIcon) {
            DrawBubblyImage("BookCloud", m_BookCloudIcon, cloudPos, actualCloudSize, dt, 1.1f, false);
        }

        if (!m_IsRecipeBookOpen) {
            // 2. KSI��KA 
            glm::vec2 bookSize = cloudSize * 1.1f;
            glm::vec2 bookPos = {
                cloudPos.x + (actualCloudSize.x - bookSize.x) * 0.5f,
                cloudPos.y + (actualCloudSize.y - bookSize.y) * 0.5f
            };

            if (DrawBubblyImage("BookIcon", m_BookIcon, bookPos, bookSize, dt, 1.15f, true, 0.35f)) {
                m_IsRecipeBookOpen = true;
                spdlog::info("UI: Otwarto ksiazke z przepisami!");
            }

            // 3. GWIAZDKI 
            if (m_BookStarsIcon) {
                DrawBubblyImage("BookStars", m_BookStarsIcon, cloudPos, actualCloudSize, dt, 1.15f, false);
            }
        }
        else {
            // -- WN�TRZE KSI��KI ---
            glm::vec2 insideSize = CalculateAspectSize(m_BookInsideIcon, gameHeight * 1.0f);
            float yOffset = 50.0f * baseScale;
            glm::vec2 insidePos = {
                gameX + (gameWidth - insideSize.x) * 0.5f,
                gameY + (gameHeight - insideSize.y) * 0.5f + yOffset
            };

            if (m_BookInsideIcon) {
                DrawBubblyImage("BookInside", m_BookInsideIcon, insidePos, insideSize, dt, 1.0f, false);
            }

            // Przycisk X do zamykania 
            glm::vec2 xSize = { 60.0f * baseScale, 60.0f * baseScale };
            glm::vec2 xPos = {
                insidePos.x + insideSize.x - xSize.x * 2.6f,
                insidePos.y + xSize.y * 2.6f
            };

            if (m_BookXIcon) {
                if (DrawBubblyImage("BookX", m_BookXIcon, xPos, xSize, dt, 1.2f, true, 0.4f)) {
                    m_IsRecipeBookOpen = false;
                    spdlog::info("UI: Zamknieto ksiazke z przepisami!");
                }
            }

            // Wy�wietlanie przepis�w
            float recipeH = 120.0f * baseScale; // Wysoko�� dla ka�dej ikony

            // 1. Zupa Pomidorowa (Rz�d 1, Kolumna 1)
            DrawRecipeIcon("TomatoSoup", m_TomatoSoupIcon, { 0.12f, 0.15f }, recipeH, insidePos, insideSize, dt);

            // 2. (Przyk�ad na przysz�o��) np. Burger (Rz�d 1, Kolumna 2)
            // DrawRecipeIcon("Burger", m_BurgerIcon, {0.25f, 0.15f}, recipeH, insidePos, insideSize, dt);

            // 3. (Przyk�ad na przysz�o��) np. Sa�atka (Rz�d 2, Kolumna 1)
            // DrawRecipeIcon("Salad", m_SaladIcon, {0.12f, 0.35f}, recipeH, insidePos, insideSize, dt);
        }
    }

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
    // Obliczamy skal� wzgl�dem wysoko�ci 1080p, z progiem bezpiecze�stwa 0.5
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
    //DrawQuestPanel(gameX, gameY, gameWidth, gameHeight, baseScale, isPlayMode);
    DrawIngredientClouds(gameX, gameY, gameWidth, gameHeight, baseScale, dt);
    DrawRecipeBook(gameX, gameY, gameWidth, gameHeight, baseScale, dt);

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