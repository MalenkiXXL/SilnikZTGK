#include "BuildModePanel.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/Application.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Layers/GuiLayer/Utils/Gui.h"
#include "CookingStation/Layers/GuiLayer/Utils/Renderer2D.h"
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Scene/PrefabSerializer.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include <spdlog/spdlog.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Layers/GuiLayer/Utils/AudioConfig.h"

static const float FLOOR_MIN_X = -15.0f;
static const float FLOOR_MAX_X = 14.0f;
static const float FLOOR_MIN_Z = -18.0f;
static const float FLOOR_MAX_Z = 18.0f;

static bool IsWithinBuildArea(const glm::vec3& pos) {
    return (pos.x >= FLOOR_MIN_X - 0.01f && pos.x <= FLOOR_MAX_X + 0.01f &&
        pos.z >= FLOOR_MIN_Z - 0.01f && pos.z <= FLOOR_MAX_Z + 0.01f);
}

static glm::vec3 ClampToBuildArea(const glm::vec3& pos) {
    glm::vec3 clamped = pos;
    if (clamped.x < FLOOR_MIN_X) clamped.x = FLOOR_MIN_X;
    if (clamped.x > FLOOR_MAX_X) clamped.x = FLOOR_MAX_X;
    if (clamped.z < FLOOR_MIN_Z) clamped.z = FLOOR_MIN_Z;
    if (clamped.z > FLOOR_MAX_Z) clamped.z = FLOOR_MAX_Z;
    return clamped;
}

static bool IsIgnoredByBuildSystem(const std::string& tag) {
    if (tag.empty()) return false;
    if (tag == "__BuildPreview__") return true;
    if (tag.find("Wielka_Pod") != std::string::npos) return true; 
    if (tag.find("Krzeslo1") != std::string::npos) return true;  
    if (tag.find("Krzeslo2") != std::string::npos) return true;   
    if (tag.find("Krzeslo3") != std::string::npos) return true;   
    if (tag.find("Krzeslo4") != std::string::npos) return true;   
    if (tag.find("Krzeslo5") != std::string::npos) return true;   
    if (tag.find("Krzeslo6") != std::string::npos) return true;   
    return false;
}

void BuildModePanel::Init(std::shared_ptr<Texture> coinIcon) {
    m_MachineEntries.clear();
    m_CoinIcon = coinIcon;

    m_MachineEntries.push_back({ "Garnek",    "assets://prefabs/pot_station.json",   AssetManager::GetTexture("assets://UI/pot.png"),   0 });
    m_MachineEntries.push_back({ "Deska",     "assets://prefabs/board_station.json", AssetManager::GetTexture("assets://UI/cuttingBoardMachine.png"), 0 });
    m_MachineEntries.push_back({ "Mikser",    "assets://prefabs/mixer.json",         AssetManager::GetTexture("assets://UI/blender.png"),   0 });
    m_MachineEntries.push_back({ "Piekarnik", "assets://prefabs/oven.json",          AssetManager::GetTexture("assets://UI/oven.png"),  0 });
    m_MachineEntries.push_back({ "Patelnia", "assets://prefabs/pan_station.json", AssetManager::GetTexture("assets://UI/pan.png"), 0 });
    m_MachineEntries.push_back({ "Ekspres", "assets://prefabs/coffee_maker.json", AssetManager::GetTexture("assets://UI/coffee_maker.png"), 0 });

    m_LeftMouseIcon = AssetManager::GetTexture("assets://UI/leftMouse.png");
    m_RightMouseIcon = AssetManager::GetTexture("assets://UI/rightMouse.png");
    m_TabIcon = AssetManager::GetTexture("assets://UI/tab.png");
}

void BuildModePanel::ForceReset() {
    m_IsActive = false;
    m_HeldMachineIndex = -1;
    m_SlideY = 0.0f;
    m_ButtonScale = 1.0f;
    m_PreviewGroup.clear();
    m_MovingGroup.clear();
    m_MovingMachineEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    m_CurrentScene = nullptr;
}

void BuildModePanel::Activate() {
    if (m_IsActive) return;
    m_IsActive = true;
    m_HeldMachineIndex = -1;
    auto activeScene = SceneManager::GetActiveScene();

    m_CurrentScene = activeScene;

    Application::Get().GetEventBus().Publish(BuildModeToggledEvent{ true });
}

void BuildModePanel::Deactivate() {
    if (!m_IsActive) return;
    m_IsActive = false;
    m_HeldMachineIndex = -1;

    m_CurrentScene = nullptr;

    auto activeScene = SceneManager::GetActiveScene();
    if (!m_PreviewGroup.empty() && activeScene) {
        for (auto& [ent, offset] : m_PreviewGroup) {
            activeScene->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ ent });
        }
        m_PreviewGroup.clear();
    }

    if (m_MovingMachineEntity.id != std::numeric_limits<std::size_t>::max() && activeScene) {
        auto* tc = activeScene->GetWorld().GetComponent<TransformComponent>(m_MovingMachineEntity);
        if (tc) tc->SetPosition(m_MovingMachineOriginalPos);

        for (auto& [groupEnt, offset] : m_MovingGroup) {
            auto* groupTc = activeScene->GetWorld().GetComponent<TransformComponent>(groupEnt);
            if (groupTc) {
                groupTc->SetPosition(m_MovingMachineOriginalPos + offset);
            }
        }

        m_MovingMachineEntity = { std::numeric_limits<std::size_t>::max(), 0 };
        m_MovingGroup.clear();
    }

    Application::Get().GetEventBus().Publish(BuildModeToggledEvent{ false });
}

void BuildModePanel::DrawButton(float gameX, float gameY, float gameW, float gameH, float baseScale, float dt, bool isBlocked) {
    if (isBlocked && !m_IsActive) return;

    auto buildBtnTex = AssetManager::GetTexture("assets://UI/buildModeButton.png");

    float bookCloudH = 210.0f * baseScale * 1.3f;
    float btnHeight = 153.0f * baseScale;
    glm::vec2 baseSize = { btnHeight * 2.5f, btnHeight };

    if (buildBtnTex && buildBtnTex->GetRendererID() != 0) {
        float aspect = (float)buildBtnTex->GetWidth() / (float)buildBtnTex->GetHeight();
        baseSize = { btnHeight * aspect, btnHeight };
    }

    glm::vec2 basePos = { gameX + 35.0f * baseScale, gameY + bookCloudH + 8.0f * baseScale };

    glm::vec2 mouse = Gui::GetMappedMousePos();
    bool inBounds = mouse.x >= basePos.x && mouse.x <= basePos.x + baseSize.x &&
        mouse.y >= basePos.y && mouse.y <= basePos.y + baseSize.y;

    static bool s_wasBuildHovered = false;
    if (inBounds && !s_wasBuildHovered && !isBlocked) {
        AudioEngine::PlayLoopingSound("assets://sounds/hover_in_game.mp3", 0.15f, false);
        s_wasBuildHovered = true;
    }
    else if (!inBounds || isBlocked) {
        s_wasBuildHovered = false;
    }

    float targetScale = inBounds ? 1.08f : 1.0f;
    m_ButtonScale += (targetScale - m_ButtonScale) * dt * 15.0f;

    glm::vec2 scaledSize = baseSize * m_ButtonScale;
    glm::vec2 scaledPos = {
        basePos.x + (baseSize.x - scaledSize.x) * 0.5f,
        basePos.y + (baseSize.y - scaledSize.y) * 0.5f
    };

    if (buildBtnTex && buildBtnTex->GetRendererID() != 0) {
        glm::vec4 tint = inBounds ? glm::vec4(0.85f, 0.85f, 0.85f, 1.0f) : glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        if (inBounds && Input::IsMouseButtonPressed(0)) {
            tint = glm::vec4(0.65f, 0.65f, 0.65f, 1.0f);
        }

        if (m_IsActive) {
            tint *= glm::vec4(0.75f, 1.0f, 0.75f, 1.0f);
        }

        Renderer2D::DrawQuad(scaledPos, scaledSize, buildBtnTex->GetRendererID(), tint, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    }
    else {
        glm::vec4 bgColor = m_IsActive ? glm::vec4(0.20f, 0.45f, 0.90f, 0.95f) : glm::vec4(0.10f, 0.12f, 0.20f, 0.82f);
        Renderer2D::DrawQuad(scaledPos, scaledSize, bgColor, 18.0f * baseScale * m_ButtonScale);

        std::string label = m_IsActive ? "[ BUILD ]" : "Build Mode";
        float textScale = 0.55f * baseScale * m_ButtonScale;
        float tw = Gui::MeasureTextWidth(label, textScale);
        float th = Gui::MeasureTextHeight(label, textScale);
        glm::vec2 textPos = { scaledPos.x + (scaledSize.x - tw) * 0.5f, scaledPos.y + (scaledSize.y - th) * 0.5f - th * 0.15f };

        Gui::DrawGuiText(label, { textPos.x + 1.5f, textPos.y + 1.5f }, textScale, { 0.0f, 0.0f, 0.0f, 0.55f });
        Gui::DrawGuiText(label, textPos, textScale, m_IsActive ? glm::vec4(0.85f, 0.95f, 1.00f, 1.0f) : glm::vec4(0.70f, 0.80f, 1.00f, 1.0f));
    }

    if (m_TabIcon && m_TabIcon->GetRendererID() != 0) {
        float tabHeight = 25.0f * baseScale * m_ButtonScale;
        float tabAspect = (float)m_TabIcon->GetWidth() / (float)m_TabIcon->GetHeight();
        glm::vec2 tabSize = { tabHeight * tabAspect, tabHeight };

        glm::vec2 tabPos = {
            scaledPos.x + (scaledSize.x - tabSize.x) * 0.5f,
            scaledPos.y + scaledSize.y + (12.0f * baseScale)
        };

        glm::vec4 tabTint = { 1.0f, 1.0f, 1.0f, 0.85f };
        Renderer2D::DrawQuad(tabPos, tabSize, m_TabIcon, tabTint, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    }

    if (inBounds) {
        Input::SetUICaptureMouse(true);
        if (Input::IsMouseButtonJustPressed(0)) Toggle();
    }
}

void BuildModePanel::DrawPanel(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt) {
    float targetSlide = m_IsActive ? 1.0f : 0.0f;
    m_SlideY += (targetSlide - m_SlideY) * std::min(dt * 14.0f, 1.0f);
    if (m_SlideY <= 0.01f) return;

    const float panelH = 200.0f * baseScale;
    float panelY = gameY + gameHeight - panelH * m_SlideY;

    auto bgTex = AssetManager::GetTexture("assets://UI/buildBackground.png");
    if (bgTex && bgTex->GetRendererID() != 0) {
        Renderer2D::DrawQuad({ gameX, panelY }, { gameWidth, panelH }, bgTex->GetRendererID(), { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    }
    else {
        Renderer2D::DrawQuad({ gameX, panelY }, { gameWidth, panelH }, { 0.06f, 0.08f, 0.14f, 0.94f }, 0.0f);
        Renderer2D::DrawQuad({ gameX, panelY }, { gameWidth, 2.0f * baseScale }, { 0.3f, 0.55f, 1.0f, 0.7f }, 0.0f);
    }

    const float iconH = 80.0f * baseScale;
    const float slotW = 120.0f * baseScale;
    const float spacing = 20.0f * baseScale;

    const int count = (int)m_MachineEntries.size();
    const float totalW = count * slotW + (count - 1) * spacing;
    const float startX = gameX + (gameWidth - totalW) * 0.5f;

    const float iconY = panelY + (panelH - iconH) * 0.5f - 10.0f * baseScale;

    glm::vec2 mouse = Gui::GetMappedMousePos();
    int currentMoney = GameManagerScript::s_Instance ? GameManagerScript::s_Instance->GetMoney() : 0;

    int currentlyHoveredSlot = -1;
    static int s_lastHoveredMachineSlot = -1;

    for (int i = 0; i < count; ++i) {
        auto& entry = m_MachineEntries[i];
        bool canAfford = currentMoney >= entry.Price;

        float actualIconW = iconH;
        if (entry.Icon && entry.Icon->GetRendererID() != 0) {
            float aspect = (float)entry.Icon->GetWidth() / (float)entry.Icon->GetHeight();
            actualIconW = iconH * aspect;
        }

        float slotStartX = startX + i * (slotW + spacing);
        float slotCenterX = slotStartX + slotW * 0.5f;

        float ix = slotCenterX - actualIconW * 0.5f;
        float iy = iconY;

        float padX = 15.0f * baseScale;
        float padY = 15.0f * baseScale;

        const bool inIcon = mouse.x >= (ix - padX) && mouse.x <= (ix + actualIconW + padX) &&
            mouse.y >= (iy - padY) && mouse.y <= (iy + iconH + padY);

        const bool isHeld = (m_HeldMachineIndex == i);

        if (inIcon && m_IsActive) {
            currentlyHoveredSlot = i;
        }

        glm::vec4 iconColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        glm::vec4 unaffordableColorNormal = { 0.55f, 0.45f, 0.45f, 0.9f };
        glm::vec4 unaffordableColorHover = { 0.75f, 0.35f, 0.35f, 0.95f };
        glm::vec4 bg = { 0.0f, 0.0f, 0.0f, 0.0f };

        if (canAfford) {
            if (isHeld) {
                iconColor = { 0.5f, 0.7f, 1.0f, 1.0f };
                bg = { 0.30f, 0.60f, 1.0f, 0.50f };
            }
            else if (inIcon) {
                iconColor = { 0.8f, 0.8f, 0.8f, 1.0f };
                bg = { 1.0f, 1.0f, 1.0f, 0.18f };
            }
        }
        else {
            if (inIcon) {
                iconColor = unaffordableColorHover;
                bg = { 0.8f, 0.2f, 0.2f, 0.3f };
            }
            else {
                iconColor = unaffordableColorNormal;
            }
        }

        if (bg.a > 0.0f) {
            Renderer2D::DrawQuad({ ix - padX, iy - padY }, { actualIconW + padX * 2.0f, iconH + padY * 2.0f }, bg, 12.0f * baseScale);
        }

        if (entry.Icon) {
            Renderer2D::DrawQuad({ ix, iy }, { actualIconW, iconH }, entry.Icon, iconColor, { 0.0f, 1.0f }, { 1.0f, 0.0f });
        }

        float priceTextScale = 0.8f * baseScale;
        std::string priceStr = std::to_string(entry.Price);
        float tw = Gui::MeasureTextWidth(priceStr, priceTextScale);

        float coinSize = 36.0f * baseScale;
        float gap = 6.0f * baseScale;
        float totalPriceW = coinSize + gap + tw;

        float priceStartX = slotCenterX - totalPriceW * 0.5f;
        float priceY = iy + iconH + 18.0f * baseScale;

        glm::vec4 coinTint = canAfford ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : (inIcon ? unaffordableColorHover : unaffordableColorNormal);

        if (m_CoinIcon && m_CoinIcon->GetRendererID() != 0) {
            Renderer2D::DrawQuad({ priceStartX, priceY - 6.0f * baseScale }, { coinSize, coinSize }, m_CoinIcon, coinTint, { 0.0f, 1.0f }, { 1.0f, 0.0f });
        }
        else {
            Renderer2D::DrawQuad({ priceStartX, priceY - 6.0f * baseScale }, { coinSize, coinSize }, coinTint, coinSize * 0.5f);
        }

        glm::vec4 affordableTextColor = { 144.0f / 255.0f, 94.0f / 255.0f, 169.0f / 255.0f, 1.0f };
        glm::vec4 priceTextColor = canAfford ? affordableTextColor : coinTint;

        Gui::DrawGuiText(priceStr, { priceStartX + coinSize + gap + 1.5f * baseScale, priceY + 1.5f * baseScale }, priceTextScale, { 0.0f, 0.0f, 0.0f, 0.8f });
        Gui::DrawGuiText(priceStr, { priceStartX + coinSize + gap, priceY }, priceTextScale, priceTextColor);

        if (inIcon && m_IsActive) {
            Input::SetUICaptureMouse(true);
            if (Input::IsMouseButtonJustPressed(0) && m_HeldMachineIndex == -1) {
                if (canAfford) {
                    AudioEngine::Play("assets://sounds/button_click_in_game.mp3");
                    m_HeldMachineIndex = i;
                    m_JustSelectedFromPanel = true;
                }
                else {
                    spdlog::warn("BuildMode: Nie stac cie na te maszyne!");
                    if (GameManagerScript::s_Instance) {
                        GameManagerScript::s_Instance->TriggerMoneyWarning();
                    }
                }
            }
        }
    }
    if (currentlyHoveredSlot != s_lastHoveredMachineSlot) {
        if (currentlyHoveredSlot != -1) {
            AudioEngine::Play("assets://sounds/hover_in_game.mp3");
        }
        s_lastHoveredMachineSlot = currentlyHoveredSlot;
    }

    bool isHoldingFromPanel = (m_HeldMachineIndex >= 0);
    bool isMovingExisting = (m_MovingMachineEntity.id != std::numeric_limits<std::size_t>::max());

    if ((isHoldingFromPanel || isMovingExisting) && m_SlideY > 0.95f) {

        float textScale = 0.85f * baseScale;
        float targetIconHeight = 75.0f * baseScale;
        float textSpacing = 15.0f * baseScale;
        float groupSpacing = 100.0f * baseScale;

        std::string placeText = "Place";
        std::string cancelText = "Cancel";

        float placeTextW = Gui::MeasureTextWidth(placeText, textScale);
        float cancelTextW = Gui::MeasureTextWidth(cancelText, textScale);
        float textH = Gui::MeasureTextHeight("P", textScale);

        float leftIconW = targetIconHeight;
        if (m_LeftMouseIcon && m_LeftMouseIcon->GetRendererID() != 0) {
            leftIconW = targetIconHeight * ((float)m_LeftMouseIcon->GetWidth() / (float)m_LeftMouseIcon->GetHeight());
        }

        float rightIconW = targetIconHeight;
        if (m_RightMouseIcon && m_RightMouseIcon->GetRendererID() != 0) {
            rightIconW = targetIconHeight * ((float)m_RightMouseIcon->GetWidth() / (float)m_RightMouseIcon->GetHeight());
        }

        float totalWidth = (leftIconW + textSpacing + placeTextW) + groupSpacing + (rightIconW + textSpacing + cancelTextW);
        float startX = gameX + (gameWidth - totalWidth) * 0.5f;
        float centerY = panelY - 45.0f * baseScale;

        glm::vec4 textColor = { 0.95f, 0.95f, 0.95f, 0.95f };
        glm::vec4 textShadow = { 0.0f, 0.0f, 0.0f, 0.6f };
        glm::vec4 iconColor = { 1.0f, 1.0f, 1.0f, 0.95f };

        float cursorX = startX;

        if (m_LeftMouseIcon) {
            Renderer2D::DrawQuad({ cursorX, centerY - targetIconHeight * 0.5f }, { leftIconW, targetIconHeight }, m_LeftMouseIcon, iconColor, { 0.0f, 1.0f }, { 1.0f, 0.0f });
        }
        cursorX += leftIconW + textSpacing;

        float labelY = centerY - textH * 0.4f;
        Gui::DrawGuiText(placeText, { cursorX + 2.0f, labelY + 2.0f }, textScale, textShadow);
        Gui::DrawGuiText(placeText, { cursorX, labelY }, textScale, textColor);
        cursorX += placeTextW + groupSpacing;

        if (m_RightMouseIcon) {
            Renderer2D::DrawQuad({ cursorX, centerY - targetIconHeight * 0.5f }, { rightIconW, targetIconHeight }, m_RightMouseIcon, iconColor, { 0.0f, 1.0f }, { 1.0f, 0.0f });
        }
        cursorX += rightIconW + textSpacing;

        Gui::DrawGuiText(cancelText, { cursorX + 2.0f, labelY + 2.0f }, textScale, textShadow);
        Gui::DrawGuiText(cancelText, { cursorX, labelY }, textScale, textColor);
    }
}

bool BuildModePanel::IsPlacementValid(std::shared_ptr<Scene>& activeScene, const glm::vec3& snappedPos) {
    std::vector<glm::ivec2> targetCells;
    std::vector<Entity> ignoredEntities;

    if (m_HeldMachineIndex >= 0) {
        for (auto& [ent, offset] : m_PreviewGroup) {
            glm::vec3 pos = snappedPos + offset;
            if (!IsWithinBuildArea(pos)) return false;
            targetCells.push_back(GridSystem::WorldToCell(pos));
            ignoredEntities.push_back(ent);
        }
    }
    else if (m_MovingMachineEntity.id != std::numeric_limits<std::size_t>::max()) {
        if (!IsWithinBuildArea(snappedPos)) return false;
        targetCells.push_back(GridSystem::WorldToCell(snappedPos));
        ignoredEntities.push_back(m_MovingMachineEntity);
        for (auto& [ent, offset] : m_MovingGroup) {
            glm::vec3 pos = snappedPos + offset;
            if (!IsWithinBuildArea(pos)) return false;
            targetCells.push_back(GridSystem::WorldToCell(pos));
            ignoredEntities.push_back(ent);
        }
    }
    else {
        return true;
    }

    if (targetCells.empty()) return true;

    auto& world = activeScene->GetWorld();
    auto* transforms = world.GetComponentVector<TransformComponent>();
    auto* tags = world.GetComponentVector<TagComponent>();
    if (!transforms) return true;

    for (size_t i = 0; i < transforms->dense.size(); ++i) {
        Entity e = transforms->reverse[i];

        bool ignored = false;
        for (auto& ig : ignoredEntities) {
            if (e.id == ig.id) { ignored = true; break; }
        }
        if (ignored) continue;

        auto* tagComp = tags ? tags->Get(e) : nullptr;

        // UŻYWAMY NOWEJ FUNKCJI IGNORUJĄCEJ
        if (tagComp && IsIgnoredByBuildSystem(tagComp->Tag)) continue;

        glm::vec3 pos = transforms->dense[i].GetPosition();
        if (pos.y < -0.2f) continue;

        glm::ivec2 cell = GridSystem::WorldToCell(pos);

        bool inTarget = false;
        for (auto& tc : targetCells) {
            if (tc == cell) { inTarget = true; break; }
        }

        if (inTarget) {
            auto* col = world.GetComponent<BoxColliderComponent>(e);
            auto* nsc = world.GetComponent<NativeScriptComponent>(e);
            bool isObstacle = false;

            if (tagComp) {
                std::string t = tagComp->Tag;
                if (t.find("Table") != std::string::npos ||
                    t.find("Tasma") != std::string::npos ||
                    t.find("tasma") != std::string::npos ||
                    t.find("Conveyor") != std::string::npos ||
                    t.find("Chair") != std::string::npos ||
                    t.find("krzeslo") != std::string::npos ||
                    t.find("Krzeslo") != std::string::npos ||
                    t.find("wydawka") != std::string::npos ||
                    t.find("Wydawka") != std::string::npos ||
                    t.find("naroznik") != std::string::npos ||
                    t.find("PlateSpawner") != std::string::npos ||
                    t.find("Garnek") != std::string::npos ||
                    t.find("Deska") != std::string::npos ||
                    t.find("Mixer") != std::string::npos ||
                    t.find("Piekarnik") != std::string::npos ||
                    t.find("Crate") != std::string::npos ||
                    t.find("Item") != std::string::npos ||
                    t.find("Plate") != std::string::npos)
                {
                    isObstacle = true;
                }
            }

            if (col || nsc || isObstacle) {
                return false;
            }
        }
    }
    return true;
}

int BuildModePanel::GetCellState(std::shared_ptr<Scene>& activeScene, const glm::vec3& snappedPos, Entity& outMachine) {
    outMachine = { std::numeric_limits<std::size_t>::max(), 0 };
    auto& world = activeScene->GetWorld();
    auto* transforms = world.GetComponentVector<TransformComponent>();
    auto* tags = world.GetComponentVector<TagComponent>();
    auto* scripts = world.GetComponentVector<NativeScriptComponent>();
    auto* colliders = world.GetComponentVector<BoxColliderComponent>();

    if (!transforms) return 0;

    glm::ivec2 targetCell = GridSystem::WorldToCell(snappedPos);
    int state = 0; // 0 = empty

    for (size_t i = 0; i < transforms->dense.size(); ++i) {
        Entity e = transforms->reverse[i];
        glm::vec3 pos = transforms->dense[i].GetPosition();

        if (pos.y < -0.2f) continue;

        if (GridSystem::WorldToCell(pos) == targetCell) {
            auto* tagComp = tags ? tags->Get(e) : nullptr;

            // UŻYWAMY NOWEJ FUNKCJI IGNORUJĄCEJ
            if (tagComp && IsIgnoredByBuildSystem(tagComp->Tag)) continue;

            auto* nsc = scripts ? scripts->Get(e) : nullptr;
            bool isMachine = false;

            if (nsc) {
                for (auto& s : nsc->Scripts) {
                    if (s.Name == "PotScript" || s.Name == "CuttingBoardScript" || s.Name == "MixerScript" ||
                        s.Name == "OvenScript" || s.Name == "CrateScript" || s.Name == "HelperCustomerScript" ||
                        s.Name == "CoffeeMakerScript") {

                        isMachine = true;
                        break;
                    }
                }
            }

            if (isMachine) {
                outMachine = e;
                return 1;
            }

            bool isObstacle = false;
            if (tagComp) {
                std::string t = tagComp->Tag;
                if (t.find("Table") != std::string::npos ||
                    t.find("Tasma") != std::string::npos ||
                    t.find("tasma") != std::string::npos ||
                    t.find("Conveyor") != std::string::npos ||
                    t.find("Chair") != std::string::npos ||
                    t.find("krzeslo") != std::string::npos ||
                    t.find("Krzeslo") != std::string::npos ||
                    t.find("wydawka") != std::string::npos ||
                    t.find("Wydawka") != std::string::npos ||
                    t.find("naroznik") != std::string::npos ||
                    t.find("PlateSpawner") != std::string::npos ||
                    t.find("Garnek") != std::string::npos ||
                    t.find("Deska") != std::string::npos ||
                    t.find("Mixer") != std::string::npos ||
                    t.find("Piekarnik") != std::string::npos ||
                    t.find("Crate") != std::string::npos ||
                    t.find("Item") != std::string::npos ||
                    t.find("Plate") != std::string::npos ||
                    t.find("NajedzonyPomocnik") != std::string::npos ||
                    t.find("HelperCustomer") != std::string::npos)
                {
                    isObstacle = true;
                }
            }
            auto* col = colliders ? colliders->Get(e) : nullptr;

            if (isObstacle || col != nullptr) {
                state = 2;
            }
        }
    }
    return state;
}

void BuildModePanel::DrawActiveGrid(std::shared_ptr<Scene>& activeScene, float gameX, float gameY, float gameW, float gameH, float baseScale) {
    if (!activeScene) return;

    auto rawMouse = Input::GetMousePosition();
    float mouseX = rawMouse.first - gameX;
    float mouseY = rawMouse.second - gameY;

    auto* camera = activeScene->GetCamera();
    if (!camera) return;

    float aspect = gameW / (gameH > 0.0f ? gameH : 1.0f);
    float orthoSize = 10.0f * (camera->Zoom / 45.0f);
    glm::mat4 proj = glm::ortho(-aspect * orthoSize, aspect * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
    glm::mat4 view = camera->GetViewMatrix();

    Ray ray = Physics::CastRayFromMouse(mouseX, mouseY, gameW, gameH, proj, view);
    glm::vec3 snappedPos(0.0f);

    bool mouseOverPanel = (rawMouse.second >= (gameY + gameH - 200.0f * baseScale));

    int hoverState = 0;

    if (std::abs(ray.Direction.y) > 1e-6f && !mouseOverPanel) {
        float t = -ray.Origin.y / ray.Direction.y;
        if (t > 0.0f) {
            glm::vec3 rawHit = ray.Origin + t * ray.Direction;

            rawHit = ClampToBuildArea(rawHit);
            snappedPos = GridSystem::SnapToGrid(rawHit);

            if (m_HeldMachineIndex >= 0 || m_MovingMachineEntity.id != std::numeric_limits<std::size_t>::max()) {
                hoverState = IsPlacementValid(activeScene, snappedPos) ? 1 : 2;
            }
            else {
                Entity dummyMachine;
                hoverState = GetCellState(activeScene, snappedPos, dummyMachine);
            }
        }
    }
    else {
        snappedPos = glm::vec3(99999.0f);
    }

    DrawGrid(proj * view, camera->Position, snappedPos, hoverState, gameX, gameY, gameW, gameH);
}

void BuildModePanel::DrawOverlay(float gameX, float gameY, float gameW, float gameH, float baseScale) {
    auto pausedTextTex = AssetManager::GetTexture("assets://UI/buildMode.png");
    if (pausedTextTex && pausedTextTex->GetRendererID() != 0) {
        float aspect = (float)pausedTextTex->GetWidth() / (float)pausedTextTex->GetHeight();
        glm::vec2 tSize = { gameW * 0.20f, (gameW * 0.20f) / aspect };
        float yPos = gameH * 0.18f;
        Renderer2D::DrawQuad({ gameX + (gameW - tSize.x) * 0.5f, gameY + yPos }, tSize, pausedTextTex->GetRendererID(), { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    }
}

void BuildModePanel::UpdatePlacement(std::shared_ptr<Scene>& activeScene, float gameX, float gameY, float gameW, float gameH, float baseScale) {
    if (!activeScene) return;

    if (Input::IsMouseButtonJustPressed(1) || Input::IsKeyPressed(GLFW_KEY_TAB)) {
        if (m_HeldMachineIndex != -1 || m_MovingMachineEntity.id != std::numeric_limits<std::size_t>::max() || !m_PreviewGroup.empty()) {
            AudioEngine::Play("assets://sounds/cancel.mp3");
        }

        if (!m_PreviewGroup.empty()) {
            for (auto& [ent, offset] : m_PreviewGroup) {
                activeScene->GetWorld().DestroyEntity(ent);
            }
            m_PreviewGroup.clear();
        }

        if (m_MovingMachineEntity.id != std::numeric_limits<std::size_t>::max()) {
            auto* tc = activeScene->GetWorld().GetComponent<TransformComponent>(m_MovingMachineEntity);
            if (tc) tc->SetPosition(m_MovingMachineOriginalPos);

            for (auto& [groupEnt, offset] : m_MovingGroup) {
                auto* groupTc = activeScene->GetWorld().GetComponent<TransformComponent>(groupEnt);
                if (groupTc) {
                    groupTc->SetPosition(m_MovingMachineOriginalPos + offset);
                }
            }

            m_MovingMachineEntity = { std::numeric_limits<std::size_t>::max(), 0 };
            m_MovingGroup.clear();
        }
        m_HeldMachineIndex = -1;
        return;
    }

    auto rawMouse = Input::GetMousePosition();
    float mouseX = rawMouse.first - gameX;
    float mouseY = rawMouse.second - gameY;

    auto* camera = activeScene->GetCamera();
    if (!camera) return;

    float aspect = gameW / (gameH > 0.0f ? gameH : 1.0f);
    float orthoSize = 10.0f * (camera->Zoom / 45.0f);
    glm::mat4 proj = glm::ortho(-aspect * orthoSize, aspect * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);

    glm::mat4 view = camera->GetViewMatrix();

    Ray ray = Physics::CastRayFromMouse(mouseX, mouseY, gameW, gameH, proj, view);
    glm::vec3 snappedPos(0.0f);
    if (std::abs(ray.Direction.y) > 1e-6f) {
        float t = -ray.Origin.y / ray.Direction.y;
        if (t > 0.0f) {
            glm::vec3 rawHit = ray.Origin + t * ray.Direction;
            rawHit = ClampToBuildArea(rawHit);
            snappedPos = GridSystem::SnapToGrid(rawHit);
        }
    }

    bool mouseOverPanel = (rawMouse.second >= (gameY + gameH - 200.0f * baseScale));

    // LOKALNA FUNKCJA POMOCNICZA DO WYSZUKANIA BLOKUJĄCEGO OBIEKTU W CELU LOGOWANIA
    auto getBlockerName = [&]() -> std::string {
        std::string blocker = "Poza plansza";
        std::vector<glm::ivec2> tCells;
        std::vector<Entity> igEnts;

        if (m_HeldMachineIndex >= 0) {
            for (auto& [ent, offset] : m_PreviewGroup) {
                glm::vec3 pos = snappedPos + offset;
                if (!IsWithinBuildArea(pos)) return blocker;
                tCells.push_back(GridSystem::WorldToCell(pos));
                igEnts.push_back(ent);
            }
        }
        else if (m_MovingMachineEntity.id != std::numeric_limits<std::size_t>::max()) {
            if (!IsWithinBuildArea(snappedPos)) return blocker;
            tCells.push_back(GridSystem::WorldToCell(snappedPos));
            igEnts.push_back(m_MovingMachineEntity);
            for (auto& [ent, offset] : m_MovingGroup) {
                glm::vec3 pos = snappedPos + offset;
                if (!IsWithinBuildArea(pos)) return blocker;
                tCells.push_back(GridSystem::WorldToCell(pos));
                igEnts.push_back(ent);
            }
        }
        else {
            if (!IsWithinBuildArea(snappedPos)) return blocker;
            tCells.push_back(GridSystem::WorldToCell(snappedPos));
        }

        auto* tfs = activeScene->GetWorld().GetComponentVector<TransformComponent>();
        auto* tgs = activeScene->GetWorld().GetComponentVector<TagComponent>();
        if (!tfs) return "Nieznany obiekt";

        for (size_t i = 0; i < tfs->dense.size(); ++i) {
            Entity e = tfs->reverse[i];
            bool ignored = false;
            for (auto& ig : igEnts) {
                if (e.id == ig.id) { ignored = true; break; }
            }
            if (ignored) continue;

            auto* tagC = tgs ? tgs->Get(e) : nullptr;

            // UŻYWAMY NOWEJ FUNKCJI IGNORUJĄCEJ
            if (tagC && IsIgnoredByBuildSystem(tagC->Tag)) continue;

            glm::vec3 pos = tfs->dense[i].GetPosition();
            if (pos.y < -0.2f) continue;

            glm::ivec2 c = GridSystem::WorldToCell(pos);
            for (auto& tc : tCells) {
                if (tc == c) {
                    if (tagC && !tagC->Tag.empty()) return tagC->Tag;
                    return "Obiekt bez nazwy (ID: " + std::to_string(e.id) + ")";
                }
            }
        }
        return "Nieznany obiekt";
        };

    if (m_HeldMachineIndex < 0 && m_MovingMachineEntity.id != std::numeric_limits<std::size_t>::max()) {
        auto& world = activeScene->GetWorld();
        auto* tc = world.GetComponent<TransformComponent>(m_MovingMachineEntity);
        if (tc) {
            glm::vec3 newPos = snappedPos;
            newPos.y = m_MovingMachineOriginalPos.y;
            tc->SetPosition(newPos);
        }

        for (auto& [groupEnt, offset] : m_MovingGroup) {
            auto* groupTc = world.GetComponent<TransformComponent>(groupEnt);
            if (groupTc) {
                glm::vec3 groupPos = snappedPos + offset;
                groupPos.y = m_MovingMachineOriginalPos.y + offset.y;
                groupTc->SetPosition(groupPos);
            }
        }

        if (Input::IsMouseButtonJustPressed(0) && !mouseOverPanel && !m_JustSelectedFromPanel) {
            if (IsPlacementValid(activeScene, snappedPos)) {
                m_MovingMachineEntity = { std::numeric_limits<std::size_t>::max(), 0 };
                m_MovingGroup.clear();
            }
            else {
                spdlog::warn("BuildMode: Miejsce zajete przez: {}", getBlockerName());
                AudioEngine::Play("assets://sounds/error.mp3");
            }
        }
        m_JustSelectedFromPanel = false;
        return;
    }

    if (m_HeldMachineIndex < 0 && m_MovingMachineEntity.id == std::numeric_limits<std::size_t>::max()) {
        if (Input::IsMouseButtonJustPressed(0) && !mouseOverPanel) {

            Entity hitMachine;
            int cellState = GetCellState(activeScene, snappedPos, hitMachine);

            if (cellState == 1 && hitMachine.id != std::numeric_limits<std::size_t>::max()) {
                auto* tc = activeScene->GetWorld().GetComponent<TransformComponent>(hitMachine);
                if (tc) {
                    m_MovingMachineOriginalPos = tc->GetPosition();
                    m_MovingMachineEntity = hitMachine;
                    m_MovingGroup.clear();
                    glm::ivec2 machineCell = GridSystem::WorldToCell(tc->GetPosition());

                    auto* transforms = activeScene->GetWorld().GetComponentVector<TransformComponent>();
                    if (transforms) {
                        for (size_t idx = 0; idx < transforms->dense.size(); ++idx) {
                            Entity candidate = transforms->reverse[idx];
                            if (candidate.id == hitMachine.id) continue;

                            auto* candTag = activeScene->GetWorld().GetComponent<TagComponent>(candidate);

                            // UŻYWAMY NOWEJ FUNKCJI IGNORUJĄCEJ W GRUPOWANIU
                            if (candTag && IsIgnoredByBuildSystem(candTag->Tag)) continue;

                            glm::vec3 candPos = transforms->dense[idx].GetPosition();
                            if (GridSystem::WorldToCell(candPos) == machineCell) {
                                m_MovingGroup.push_back({ candidate, candPos - tc->GetPosition() });
                            }
                        }
                    }
                }
            }
            else if (cellState == 2) {
                // LOGIKA: KLIKNIĘTO Z PUSTYMI RĘKAMI W CZERWONY KAFEL (PRZESZKODĘ)
                std::string obstacleName = "Nieznany obiekt";
                auto* tfs = activeScene->GetWorld().GetComponentVector<TransformComponent>();
                auto* tgs = activeScene->GetWorld().GetComponentVector<TagComponent>();

                if (tfs) {
                    glm::ivec2 targetCell = GridSystem::WorldToCell(snappedPos);
                    for (size_t idx = 0; idx < tfs->dense.size(); ++idx) {
                        Entity candidate = tfs->reverse[idx];
                        auto* candTag = tgs ? tgs->Get(candidate) : nullptr;

                        // UŻYWAMY NOWEJ FUNKCJI IGNORUJĄCEJ
                        if (candTag && IsIgnoredByBuildSystem(candTag->Tag)) continue;

                        if (tfs->dense[idx].GetPosition().y < -0.2f) continue;

                        if (GridSystem::WorldToCell(tfs->dense[idx].GetPosition()) == targetCell) {
                            if (candTag && !candTag->Tag.empty()) obstacleName = candTag->Tag;
                            break;
                        }
                    }
                }
                spdlog::warn("BuildMode: Kliknieto w przeszkode: {}", obstacleName);
                AudioEngine::Play("assets://sounds/error.mp3");
            }
        }
        return;
    }

    if (m_HeldMachineIndex < 0 || m_HeldMachineIndex >= (int)m_MachineEntries.size()) return;
    auto& entry = m_MachineEntries[m_HeldMachineIndex];
    auto& world = activeScene->GetWorld();

    if (m_PreviewGroup.empty()) {
        std::vector<Entity> spawnedParts = PrefabSerializer::Deserialize(activeScene.get(), entry.PrefabPath, snappedPos);
        for (Entity ent : spawnedParts) {
            auto* tc = world.GetComponent<TransformComponent>(ent);
            m_PreviewGroup.push_back({ ent, tc ? (tc->GetPosition() - snappedPos) : glm::vec3(0.0f) });
            auto* tagComp = world.GetComponent<TagComponent>(ent);
            if (tagComp) tagComp->Tag = "__BuildPreview__";
            world.RemoveComponent<NativeScriptComponent>(ent);
            world.RemoveComponent<BoxColliderComponent>(ent);
        }
    }
    else {
        for (auto& [ent, offset] : m_PreviewGroup) {
            auto* tc = world.GetComponent<TransformComponent>(ent);
            if (tc) {
                glm::vec3 newPos = snappedPos + offset;
                newPos.y = snappedPos.y + offset.y;
                tc->SetPosition(newPos);
            }
        }
    }

    if (Input::IsMouseButtonJustPressed(0) && !mouseOverPanel && !m_JustSelectedFromPanel) {
        if (IsPlacementValid(activeScene, snappedPos)) {
            if (GameManagerScript::s_Instance && GameManagerScript::s_Instance->GetMoney() >= entry.Price) {
                GameManagerScript::s_Instance->SpendMoney(entry.Price);

                for (auto& [ent, _] : m_PreviewGroup) {
                    world.DestroyEntity(ent);
                }
                m_PreviewGroup.clear();

                std::vector<Entity> placedEntities = PrefabSerializer::Deserialize(activeScene.get(), entry.PrefabPath, snappedPos);
                for (Entity ent : placedEntities) {
                    auto* tagComp = world.GetComponent<TagComponent>(ent);
                    if (tagComp) tagComp->Tag = entry.Label;
                }
                m_HeldMachineIndex = -1;
            }
            else {
                for (auto& [ent, _] : m_PreviewGroup) {
                    world.DestroyEntity(ent);
                }
                m_PreviewGroup.clear();
                m_HeldMachineIndex = -1;

                if (GameManagerScript::s_Instance) {
                    GameManagerScript::s_Instance->TriggerMoneyWarning();
                }
            }
        }
        else {
            spdlog::warn("BuildMode: Miejsce zajete przez: {}", getBlockerName());
            AudioEngine::Play("assets://sounds/error.mp3");
        }
    }
    m_JustSelectedFromPanel = false;
}

void BuildModePanel::DrawGrid(const glm::mat4& viewProj3D, const glm::vec3& camPos, const glm::vec3& hoverPos, int hoverState, float gameX, float gameY, float gameW, float gameH) {
    const float cell = GridSystem::CELL_SIZE;
    const float t = 0.06f;

    static glm::ivec2 s_lastHoveredCell = { -999, -999 };

    glm::vec4 lineColor = { 0.6f, 0.6f, 0.6f, 0.40f };
    glm::vec4 hoverColor;

    if (hoverState == 1) {
        hoverColor = glm::vec4(0.3f, 0.85f, 0.3f, 0.6f);
    }
    else if (hoverState == 2) {
        hoverColor = glm::vec4(0.9f, 0.2f, 0.2f, 0.65f);
    }
    else {
        hoverColor = glm::vec4(0.3f, 0.75f, 1.0f, 0.55f);
    }

    int startX = (int)std::floor(FLOOR_MIN_X / cell);
    int endX = (int)std::ceil(FLOOR_MAX_X / cell);
    int startZ = (int)std::floor(FLOOR_MIN_Z / cell);
    int endZ = (int)std::ceil(FLOOR_MAX_Z / cell);

    auto windowSize = Input::GetWindowSize();
    int vpX = (int)gameX;
    int vpY = (int)(windowSize.second - (gameY + gameH));
    int vpW = (int)gameW;
    int vpH = (int)gameH;

    glViewport(vpX, vpY, vpW, vpH);

    auto FlatQuad = [](const glm::vec3& center, float sx, float sz) -> glm::mat4 {
        return glm::translate(glm::mat4(1.0f), center) * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), { 1.f, 0.f, 0.f }) * glm::scale(glm::mat4(1.0f), { sx, sz, 1.f }) * glm::translate(glm::mat4(1.0f), { -0.5f, -0.5f, 0.f });
        };

    Renderer2D::EndScene();
    Renderer2D::BeginScene(viewProj3D);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (hoverPos.x < 90000.0f && IsWithinBuildArea(hoverPos)) {
        glm::ivec2 hCell = GridSystem::WorldToCell(hoverPos);

        if (hCell != s_lastHoveredCell) {
            if (m_HeldMachineIndex >= 0 || m_MovingMachineEntity.id != std::numeric_limits<std::size_t>::max()) {
                AudioEngine::Play("assets://sounds/hover_in_game.mp3");
            }
            s_lastHoveredCell = hCell;
        }

        glm::vec3  hCenter = { (hCell.x + 0.5f) * cell, 0.01f, (hCell.y + 0.5f) * cell };
        Renderer2D::DrawQuad(FlatQuad(hCenter, cell, cell), hoverColor);
    }

    float realMinX = startX * cell;
    float realMaxX = endX * cell;
    float lenX = realMaxX - realMinX;
    float cX = (realMinX + realMaxX) * 0.5f;

    for (int cz = startZ; cz <= endZ; ++cz) {
        Renderer2D::DrawQuad(FlatQuad({ cX, 0.01f, cz * cell }, lenX, t), lineColor);
    }

    float realMinZ = startZ * cell;
    float realMaxZ = endZ * cell;
    float lenZ = realMaxZ - realMinZ;
    float cZ = (realMinZ + realMaxZ) * 0.5f;

    for (int cx = startX; cx <= endX; ++cx) {
        Renderer2D::DrawQuad(FlatQuad({ cx * cell, 0.01f, cZ }, t, lenZ), lineColor);
    }

    Renderer2D::EndScene();

    glViewport(0, 0, (int)windowSize.first, (int)windowSize.second);
}