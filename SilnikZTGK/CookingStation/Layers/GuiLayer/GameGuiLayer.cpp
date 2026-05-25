#include "GameGuiLayer.h"
#include "EditorGuiLayer.h"
#include "Gui.h"
#include "Renderer2D.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Events/EditorEvents.h" 
#include "CookingStation/Core/Application.h"
#include "CookingStation/json.hpp"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include <algorithm> 
#include "CookingStation/Scripts/DragAndDropScript.h"
#include "CookingStation/Scripts/Quests/DeliveryBoothScript.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include "CookingStation/Core/GameProgress.h"
#include "CookingStation/Core/VFS/VFS.h"

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

    void DrawWrappedGuiText(const std::string& text, glm::vec2 startPos, float scale, const glm::vec4& color, float lineSpacing = 24.0f, size_t maxCharsPerLine = 36) {
        std::stringstream ss(text);
        std::string word;
        std::string currentLine = "";
        glm::vec2 currentPos = startPos;

        while (ss >> word) {
            if (currentLine.length() + word.length() + 1 > maxCharsPerLine) {
                if (!currentLine.empty()) {
                    // Cień tekstu (czarny z alfą dla idealnego kontrastu)
                    Gui::DrawGuiText(currentLine, { currentPos.x + 2.0f, currentPos.y + 2.0f }, scale, { 0.0f, 0.0f, 0.0f, 0.85f });
                    // Tekst główny
                    Gui::DrawGuiText(currentLine, currentPos, scale, color);
                    currentPos.y += lineSpacing;
                }
                currentLine = word;
            }
            else {
                if (!currentLine.empty()) currentLine += " ";
                currentLine += word;
            }
        }
        if (!currentLine.empty()) {
            Gui::DrawGuiText(currentLine, { currentPos.x + 2.0f, currentPos.y + 2.0f }, scale, { 0.0f, 0.0f, 0.0f, 0.85f });
            Gui::DrawGuiText(currentLine, currentPos, scale, color);
        }
    }
}

void GameGuiLayer::OnAttach() {


#ifdef CS_DISTRIBUTION
    // Skoro EditorGuiLayer jest wyłączony, my musimy odpalić system tekstów
    Gui::Init("assets://fonts/ARIAL.TTF", 32);
#endif

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
    m_CoinIcon = AssetManager::GetTexture("assets://UI/coin.png");

}


bool GameGuiLayer::DrawBubblyImage(const std::string& id,
    const std::shared_ptr<Texture>& icon,
    glm::vec2 basePos, glm::vec2 baseSize,
    float dt, float hoverScale,
    bool darkenOnHover, float hitRadiusMultiplier,
    glm::vec4 tintColor, bool* outIsHovered)
{
    if (!icon) return false;

    auto& state = m_BubblyStates[id];

    glm::vec2 mousePos = Gui::GetMappedMousePos();

    float animSpeed = 15.0f;

    glm::vec2 center = { basePos.x + baseSize.x * 0.5f, basePos.y + baseSize.y * 0.5f };
    float hitRadius = std::min(baseSize.x, baseSize.y) * hitRadiusMultiplier;

    float distX = mousePos.x - center.x;
    float distY = mousePos.y - center.y;

    bool isHovered = (distX * distX + distY * distY) <= (hitRadius * hitRadius);

    if (outIsHovered != nullptr) {
        *outIsHovered = isHovered;
    }

    float targetScale = isHovered ? hoverScale : 1.0f;

    glm::vec4 targetColor = (isHovered && darkenOnHover)
        ? tintColor * glm::vec4(0.8f, 0.8f, 0.8f, 1.0f)
        : tintColor;

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

// Funkcja do rysowania ikony przepisu w ksiazce kucharskiej, z obsluga odblokowania 
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
    // 1. Sprawdzamy czy budka i aktywne zadanie w ogóle istnieją
    if (!DeliveryBoothScript::s_Instance || !DeliveryBoothScript::s_Instance->HasActiveQuest()) {
        return;
    }

    const auto* activeQuest = DeliveryBoothScript::s_Instance->GetActiveQuest();
    if (!activeQuest) return;

    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    if (!activeScene || !activeScene->GetCamera()) return;

    // 2. Pobieramy TransformComponent budki i wyciągamy jej globalną pozycję 3D ze świata gry
    auto* transform = activeScene->GetWorld().GetComponent<TransformComponent>(DeliveryBoothScript::s_Instance->GetEntity());
    if (!transform) return;

    glm::vec3 boothGlobalPos = glm::vec3(transform->WorldMatrix[3][0], transform->WorldMatrix[3][1], transform->WorldMatrix[3][2]);
    // Unosimy punkt projekcji delikatnie w górę (oś Y), aby chmurka unosiła się dumnie NAD dachem budki
    boothGlobalPos.y += 2.5f;

    // 3. Rekonstruujemy macierze rzutowania 3D kamery identycznie jak w silniku renderującym
    auto* camera = activeScene->GetCamera();
    glm::mat4 view = camera->GetViewMatrix();
    float currentAspect = gameWidth / (gameHeight > 0.0f ? gameHeight : 1.0f);
    float orthoSize = camera->OrthoSize;
    glm::mat4 proj3D = glm::ortho(-currentAspect * orthoSize, currentAspect * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);

    glm::mat4 viewProjection3D = proj3D * view;

    // Przeliczamy współrzędne 3D świata na przestrzeń Clip Space, a następnie na piksele ekranu 2D (NDC mapping)
    glm::vec4 clipSpacePos = viewProjection3D * glm::vec4(boothGlobalPos, 1.0f);
    if (clipSpacePos.w == 0.0f) return; // Zabezpieczenie przed dzieleniem przez zero

    glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;

    // Pozycja środka budki zmapowana na Twój ujednolicony system współrzędnych GUI
    float boothScreenX = gameX + (ndcSpacePos.x + 1.0f) * 0.5f * gameWidth;
    float boothScreenY = gameY + (1.0f - ndcSpacePos.y) * 0.5f * gameHeight;

    // 4. LOGIKA HOVER: Pobieramy pozycję myszy i sprawdzamy, czy gracz najechał na budkę
    glm::vec2 mousePos = Gui::GetMappedMousePos();
    float hoverRadius = 90.0f * baseScale; // Promień wykrywania najechana wokół budki

    float dx = mousePos.x - boothScreenX;
    float dy = mousePos.y - (boothScreenY + 60.0f * baseScale); // Lekki offset korekcyjny myszy
    bool isMouseOverBooth = (dx * dx + dy * dy) <= (hoverRadius * hoverRadius);

    // Jeśli gracz NIE najechał myszką na budkę, natychmiast przerywamy rysowanie (chmurka jest schowana!)
    if (!isMouseOverBooth) {
        return;
    }

    // 5. RYSOWANIE PŁYWAJĄCEJ CHMURKI QUESTOWEJ
    glm::vec2 cloudSize = { 340.0f * baseScale, 225.0f * baseScale };
    glm::vec2 cloudPos = { boothScreenX - cloudSize.x * 0.5f, boothScreenY - cloudSize.y };

    // Ograniczenia ekranowe krawędzi okna gry
    if (cloudPos.x < gameX + 10.0f) cloudPos.x = gameX + 10.0f;
    if (cloudPos.x + cloudSize.x > gameX + gameWidth - 10.0f) cloudPos.x = gameX + gameWidth - cloudSize.x;
    if (cloudPos.y < gameY + 10.0f) cloudPos.y = gameY + 10.0f;

    // Rysowanie tła chmurki
    Gui::Panel(cloudPos, cloudSize, { 0.08f, 0.08f, 0.1f, 0.96f }, 20.0f * baseScale);

    float textX = cloudPos.x + 16.0f * baseScale;
    float currentY = cloudPos.y + 15.0f * baseScale;
    float spacing = 24.0f * baseScale;

    // Nagłówek okienka
    Gui::DrawGuiText("AKTYWNY EVENT PRODUKCYJNY AI", { textX, currentY }, 0.42f * baseScale, { 1.0f, 0.5f, 0.1f, 1.0f });
    currentY += spacing + 5.0f * baseScale;

    // Tytuł wylosowany z newsa
    Gui::DrawGuiText(activeQuest->Title, { textX, currentY }, 0.62f * baseScale, { 1.0f, 0.85f, 0.2f, 1.0f });
    currentY += spacing + 8.0f * baseScale;

    // Krótki i deterministyczny opis z automatycznym zawijaniem wierszy
    DrawWrappedGuiText(activeQuest->Description, { textX, currentY }, 0.60f * baseScale, { 0.9f, 0.9f, 0.9f, 1.0f }, spacing, 30);

    float footerY = cloudPos.y + cloudSize.y - 60.0f * baseScale;

    // Cel z mapowaniem konkretnego ID potrawy
    std::string goalStr = "Wymagane: " + activeQuest->DishId + " (" +
        std::to_string(activeQuest->PortionsDelivered) + " / " +
        std::to_string(activeQuest->PortionsRequired) + " szt.)";
    Gui::DrawGuiText(goalStr, { textX, footerY }, 0.60f * baseScale, { 0.3f, 1.0f, 0.4f, 1.0f });

    // Nagroda finansowa/rzeczowa
    Gui::DrawGuiText("Nagroda: " + activeQuest->Reward, { textX, footerY + 24.0f * baseScale }, 0.42f * baseScale, { 0.3f, 0.8f, 1.0f, 1.0f });
}

void GameGuiLayer::DrawIngredientClouds(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt) {
    if (m_CornerIcon) {
        glm::vec2 baseIconSize = CalculateAspectSize(m_CornerIcon, gameHeight * 0.30f);

        glm::vec2 leftPosBase = { gameX, gameY + gameHeight - baseIconSize.y };
        glm::vec2 rightPosBase = { gameX + gameWidth - baseIconSize.x, gameY + gameHeight - baseIconSize.y };

        // Rusyjemy chmury
        DrawBubblyImage("CloudLeft", m_CornerIcon, leftPosBase, baseIconSize, dt, 1.15f, false, 0.55f);
        DrawBubblyImage("CloudRight", m_CornerIcon, rightPosBase, baseIconSize, dt, 1.15f, false);

        // Rysujemy skladniki na lewej chmurze
        if (m_TomatoIcon) {
            float tomatoBaseH = baseIconSize.y * 0.3f;
            glm::vec2 tomatoBaseSize = { tomatoBaseH, tomatoBaseH };
            glm::vec2 tomatoBasePos = {
                leftPosBase.x + (baseIconSize.x * 0.5f) - (tomatoBaseSize.x * 0.5f),
                leftPosBase.y + (baseIconSize.y * 0.5f) - (tomatoBaseSize.y * 0.5f)
            };

            int tomatoCount = 0;
            std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
            if (activeScene && activeScene->GetState() == SceneState::Play)
            {
                if (GameManagerScript::s_Instance) {
                    tomatoCount = GameManagerScript::s_Instance->GetIngredientCount(IngredientType::Tomato);
                }
            }
            bool showCountText = true;

            if (DrawIngredientIcon("BtnTomato", m_TomatoIcon, tomatoBasePos, tomatoBaseSize, dt, baseScale, tomatoCount, showCountText)) {
                spdlog::info("UI: Wyciagnieto pomidora!");
                DragAndDropScript::StartDrag(IngredientType::Tomato, "assets://models/skladniki/pomidor/pomidor.gltf");
            }
        }
    }
}

void GameGuiLayer::DrawRecipeBook(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt) {
    // --- KSIAZKA Z PRZEPISAMI ---
    if (m_BookIcon) {
        glm::vec2 cloudSize = { 280.0f * baseScale, 280.0f * baseScale };
        glm::vec2 cloudPos = { gameX + 20.0f * baseScale, gameY + 20.0f * baseScale };
        glm::vec2 actualCloudSize = cloudSize * 1.3f;

        // 1. CHMURKA
        if (m_BookCloudIcon) {
            DrawBubblyImage("BookCloud", m_BookCloudIcon, cloudPos, actualCloudSize, dt, 1.1f, false);
        }

        if (!m_IsRecipeBookOpen) {
            // 2. KSIAZKA 
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
            // -- WNETRZE KSIAZKI ---
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

            // Wyswietlanie przepisow
            float recipeH = 120.0f * baseScale; // Wysokosc dla kazdej ikony

            // 1. Zupa Pomidorowa (Rzad 1, Kolumna 1)
            DrawRecipeIcon("TomatoSoup", m_TomatoSoupIcon, { 0.12f, 0.15f }, recipeH, insidePos, insideSize, dt);
        }
    }

}

void GameGuiLayer::DrawIconWithText(const std::string& text,
                                    const std::shared_ptr<Texture>& iconTex,
                                    const glm::vec2& textPos,
                                    float textScale,
                                    float baseScale,
                                    float dt)
{
    if (!iconTex) return;

    // 1. Wymiary ikony
    float coinH = 80.0f * baseScale;
    glm::vec2 coinSize = { coinH, coinH };

    // 2. Pomiar tekstu
    float textHeight = Gui::MeasureTextHeight(text, textScale);
    float baselineOffset = 32.0f * 0.8f * textScale;

    // 3. Obliczenie idealnego środka (z uwzględnieniem poprawki)
    float textCenterY = textPos.y + baselineOffset - (textHeight * 0.5f);

    // Tutaj wrzuć wartość manualNudge, którą wypracowałeś w poprzednim kroku
    float manualNudge = 0.0f;

    glm::vec2 coinPos = {
            textPos.x - coinSize.x - 8.0f * baseScale,
            textCenterY - (coinSize.y * 0.5f) + (manualNudge * baseScale)
    };

    // 4. Renderowanie ikony i tekstu (cień + przód)
    DrawBubblyImage("CoinIcon", iconTex, coinPos, coinSize, dt, 1.05f, false);

    // Cień tekstu
    Gui::DrawGuiText(text, { textPos.x + 2.0f, textPos.y + 2.0f }, textScale, { 0.0f, 0.0f, 0.0f, 0.85f });
    // Główny tekst
    Gui::DrawGuiText(text, textPos, textScale, { 1.0f, 0.95f, 0.3f, 1.0f });
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
#ifdef CS_DISTRIBUTION
    // W grze standalone GUI zajmuje CAŁY ekran — brak paneli edytora
    float gameX = 0.0f;
    float gameY = 0.0f;
    float gameWidth = m_ViewportWidth;
    float gameHeight = m_ViewportHeight;
#else
    // W edytorze zostawiamy miejsce na lewy/prawy panel i toolbar
    float gameX = 200.0f;
    float gameY = 30.0f;
    float gameWidth = m_ViewportWidth - 500.0f;
    float gameHeight = m_ViewportHeight - 230.0f;
#endif

    if (gameWidth <= 0.0f || gameHeight <= 0.0f) return;

    // --- DYNAMICZNA SKALA ---
    float baseScale = std::max(gameHeight / 1080.0f, 0.5f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);
    Renderer2D::BeginScene(uiProj);

    // --- SCISSOR TEST ---
    Renderer2D::EndScene();
    glEnable(GL_SCISSOR_TEST);
    int scissorY = (int)(m_ViewportHeight - (gameY + gameHeight));
    glScissor((int)gameX, scissorY, (int)gameWidth, (int)gameHeight);

    Renderer2D::BeginScene(uiProj);
    DrawQuestPanel(gameX, gameY, gameWidth, gameHeight, baseScale, isPlayMode);
    DrawIngredientClouds(gameX, gameY, gameWidth, gameHeight, baseScale, dt);
    DrawRecipeBook(gameX, gameY, gameWidth, gameHeight, baseScale, dt);

    // --- PIENIĄDZE ---
    if (m_CoinIcon) {
        int money = GameManagerScript::s_Instance ? GameManagerScript::s_Instance->GetMoney() : 0;

        if (money != m_LastMoney) {
            m_MoneyStr = std::to_string(money);
            m_LastMoney = money;
        }

        float textScale = 2.0f * baseScale;
        float textWidth = Gui::MeasureTextWidth(m_MoneyStr, textScale);
        glm::vec2 textPos = {
                gameX + gameWidth * 0.97f - textWidth,
                gameY + gameHeight * 0.02f
        };

        DrawIconWithText(m_MoneyStr, m_CoinIcon, textPos, textScale, baseScale, dt);
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

        // POPRAWKA: synchronizujemy przestrzeń logiczną Gui z nowym rozmiarem okna
        // aby GetMappedMousePos() działało poprawnie po zmianie rozdzielczości
        Gui::SetScreenSize(m_ViewportWidth, m_ViewportHeight);
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

// ZMIANA: Przebudowano funkcję ładującą JSON tak, aby korzystała z VFS
void GameGuiLayer::ReloadQuests() {
    m_CurrentQuests.clear();
    
    // Używamy VFS do wyciągnięcia bajtów z pliku
    std::vector<uint8_t> fileData = VFS::ReadFile("assets://wygenerowane_quests.json");
    
    if (!fileData.empty()) {
        try {
            // Biblioteka nlohmann::json potrafi przetworzyć std::vector bezpośrednio!
            nlohmann::json data = nlohmann::json::parse(fileData);
            for (auto& q : data) {
                m_CurrentQuests.push_back({
                    q.value("title", "Brak tytulu"),
                    q.value("description", "Brak opisu"),
                    q.value("portions", 0),
                    q.value("reward", "Brak nagrody")
                    });
            }
            m_CurrentQuestIndex = 0;
            spdlog::info("GameUiLayer: Questy zaladowane responsywnie przez VFS.");
        }
        catch (...) {
            spdlog::error("GameUiLayer: Blad JSON podczas parsowania z VFS.");
        }
    }
    else {
        spdlog::error("GameUiLayer: Nie udalo sie wczytac pliku wygenerowane_quests.json przez VFS.");
    }
}