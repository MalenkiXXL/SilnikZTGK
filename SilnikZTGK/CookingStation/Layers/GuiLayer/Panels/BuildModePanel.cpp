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
#include <spdlog/spdlog.h>
void BuildModePanel::Init() {
    m_MachineEntries.clear(); 

    m_MachineEntries.push_back({ "Garnek",    "assets://prefabs/pot_station.json",   AssetManager::GetTexture("assets://UI/pot.png") });
    m_MachineEntries.push_back({ "Deska",     "assets://prefabs/board_station.json", AssetManager::GetTexture("assets://UI/Flour.png") });
    m_MachineEntries.push_back({ "Mikser",    "assets://prefabs/mixer.json",         AssetManager::GetTexture("assets://UI/pot.png") });
    m_MachineEntries.push_back({ "Piekarnik", "assets://prefabs/oven.json",          AssetManager::GetTexture("assets://UI/oven.png") });
}

void BuildModePanel::Activate() {
    if (m_IsActive) return;
    m_IsActive = true;
    m_HeldMachineIndex = -1;
    auto activeScene = SceneManager::GetActiveScene();
    if (activeScene) activeScene->SetState(SceneState::Pause);
    Application::Get().GetEventBus().Publish(GamePausedEvent{});
}

void BuildModePanel::Deactivate() {
    if (!m_IsActive) return;
    m_IsActive = false;
    m_HeldMachineIndex = -1;

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
        m_MovingMachineEntity = { std::numeric_limits<std::size_t>::max(), 0 };
        m_MovingGroup.clear();
    }

    if (activeScene) activeScene->SetState(SceneState::Play);
    Application::Get().GetEventBus().Publish(GameResumedEvent{});
}

void BuildModePanel::DrawButton(float gameX, float gameY, float gameW, float gameH, float baseScale, float dt, bool isBlocked) {
    if (isBlocked && !m_IsActive) return;

    float bookCloudH = 210.0f * baseScale * 1.3f;
    glm::vec2 baseSize = { 180.0f * baseScale, 70.0f * baseScale };
    glm::vec2 basePos = { gameX + 14.0f * baseScale, gameY + bookCloudH + 8.0f * baseScale };

    glm::vec2 mouse = Gui::GetMappedMousePos();
    bool inBounds = mouse.x >= basePos.x && mouse.x <= basePos.x + baseSize.x &&
        mouse.y >= basePos.y && mouse.y <= basePos.y + baseSize.y;

    float targetScale = inBounds ? 1.08f : 1.0f; 
    m_ButtonScale += (targetScale - m_ButtonScale) * dt * 15.0f;

    glm::vec2 scaledSize = baseSize * m_ButtonScale;
    glm::vec2 scaledPos = {
        basePos.x + (baseSize.x - scaledSize.x) * 0.5f,
        basePos.y + (baseSize.y - scaledSize.y) * 0.5f
    };

    glm::vec4 bgColor = m_IsActive ? glm::vec4(0.20f, 0.45f, 0.90f, 0.95f) : glm::vec4(0.10f, 0.12f, 0.20f, 0.82f);
    Renderer2D::DrawQuad(scaledPos, scaledSize, bgColor, 18.0f * baseScale * m_ButtonScale);

    std::string label = m_IsActive ? "[ BUILD ]" : "Build Mode";
    float textScale = 0.55f * baseScale * m_ButtonScale;
    float tw = Gui::MeasureTextWidth(label, textScale);
    float th = Gui::MeasureTextHeight(label, textScale);
    glm::vec2 textPos = { scaledPos.x + (scaledSize.x - tw) * 0.5f, scaledPos.y + (scaledSize.y - th) * 0.5f - th * 0.15f };

    Gui::DrawGuiText(label, { textPos.x + 1.5f, textPos.y + 1.5f }, textScale, { 0.0f, 0.0f, 0.0f, 0.55f });
    Gui::DrawGuiText(label, textPos, textScale, m_IsActive ? glm::vec4(0.85f, 0.95f, 1.00f, 1.0f) : glm::vec4(0.70f, 0.80f, 1.00f, 1.0f));

    if (inBounds) {
        Input::SetUICaptureMouse(true);
        if (Input::IsMouseButtonJustPressed(0)) Toggle();
    }
}
void BuildModePanel::DrawPanel(float gameX, float gameY, float gameWidth, float gameHeight, float baseScale, float dt) {
    float targetSlide = m_IsActive ? 1.0f : 0.0f;
    m_SlideY += (targetSlide - m_SlideY) * std::min(dt * 14.0f, 1.0f);
    if (m_SlideY <= 0.01f) return;

    const float panelH = 130.0f * baseScale;
    float panelY = gameY + gameHeight - panelH * m_SlideY;

    Renderer2D::DrawQuad({ gameX, panelY }, { gameWidth, panelH }, { 0.06f, 0.08f, 0.14f, 0.94f }, 0.0f);
    Renderer2D::DrawQuad({ gameX, panelY }, { gameWidth, 2.0f * baseScale }, { 0.3f, 0.55f, 1.0f, 0.7f }, 0.0f);

    const float iconH = 80.0f * baseScale;
    const float iconW = iconH;
    const float spacing = 28.0f * baseScale;
    const int count = (int)m_MachineEntries.size();
    const float totalW = count * iconW + (count - 1) * spacing;
    const float startX = gameX + (gameWidth - totalW) * 0.5f;
    const float iconY = panelY + (panelH - iconH) * 0.5f;

    glm::vec2 mouse = Gui::GetMappedMousePos();

    for (int i = 0; i < count; ++i) {
        const float ix = startX + i * (iconW + spacing);
        const float iy = iconY;

        const bool inIcon = mouse.x >= ix && mouse.x <= ix + iconW && mouse.y >= iy && mouse.y <= iy + iconH;
        const bool isHeld = (m_HeldMachineIndex == i);

        glm::vec4 bg = isHeld ? glm::vec4(0.30f, 0.60f, 1.00f, 0.50f) : (inIcon ? glm::vec4(1.00f, 1.00f, 1.00f, 0.18f) : glm::vec4(1.00f, 1.00f, 1.00f, 0.07f));
        Renderer2D::DrawQuad({ ix, iy }, { iconW, iconH }, bg, 8.0f * baseScale);

        auto& entry = m_MachineEntries[i];
        if (entry.Icon) {
            Renderer2D::DrawQuad({ ix, iy }, { iconW, iconH }, entry.Icon, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
        }

        float ls = 0.42f * baseScale;
        float lw = Gui::MeasureTextWidth(entry.Label, ls);
        Gui::DrawGuiText(entry.Label, { ix + (iconW - lw) * 0.5f, iy + iconH + 5.0f * baseScale }, ls, { 0.80f, 0.90f, 1.00f, 1.0f });

        if (inIcon && m_IsActive) {
            Input::SetUICaptureMouse(true);
            if (Input::IsMouseButtonJustPressed(0) && m_HeldMachineIndex == -1) {
                m_HeldMachineIndex = i;
                m_JustSelectedFromPanel = true;
            }
        }
    }

    if (m_HeldMachineIndex >= 0 && m_SlideY > 0.95f) {
        const std::string hint = "LPM: postaw   PPM / Tab: anuluj";
        float hs = 0.44f * baseScale;
        float hw = Gui::MeasureTextWidth(hint, hs);
        Gui::DrawGuiText(hint, { gameX + (gameWidth - hw) * 0.5f, panelY - 26.0f * baseScale }, hs, { 0.95f, 0.95f, 0.60f, 0.90f });
    }
}

void BuildModePanel::UpdatePlacement(std::shared_ptr<Scene>& activeScene) {
    if (!activeScene) return;

    if (Input::IsMouseButtonJustPressed(1)) {
        if (!m_PreviewGroup.empty()) {
            for (auto& [ent, offset] : m_PreviewGroup) {
                activeScene->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ ent });
            }
            m_PreviewGroup.clear();
        }

        if (m_MovingMachineEntity.id != std::numeric_limits<std::size_t>::max()) {
            auto* tc = activeScene->GetWorld().GetComponent<TransformComponent>(m_MovingMachineEntity);
            if (tc) tc->SetPosition(m_MovingMachineOriginalPos);
            m_MovingMachineEntity = { std::numeric_limits<std::size_t>::max(), 0 };
            m_MovingGroup.clear();
        }
        m_HeldMachineIndex = -1;
        return;
    }

    auto mousePos = Input::GetMousePosition();
    auto windowSize = Input::GetWindowSize();
    float mouseX = mousePos.first;
    float mouseY = mousePos.second;
    float viewW = (float)windowSize.first;
    float viewH = (float)windowSize.second;

#ifndef CS_DISTRIBUTION
    mouseX -= 200.0f;
    mouseY -= 30.0f;
    viewW -= 500.0f;
    viewH -= 230.0f;
#endif

    auto* camera = activeScene->GetCamera();
    if (!camera) return;

    float aspect = viewW / (viewH > 0.0f ? viewH : 1.0f);
    float orthoSize = 10.0f * (camera->Zoom / 45.0f);
    glm::mat4 proj = glm::ortho(-aspect * orthoSize, aspect * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
    glm::mat4 view = camera->GetViewMatrix();

    Ray ray = Physics::CastRayFromMouse(mouseX, mouseY, viewW, viewH, proj, view);
    glm::vec3 snappedPos(0.0f);
    if (std::abs(ray.Direction.y) > 1e-6f) {
        float t = -ray.Origin.y / ray.Direction.y;
        if (t > 0.0f) snappedPos = GridSystem::SnapToGrid(ray.Origin + t * ray.Direction);
    }

    auto rawMouse = Input::GetMousePosition();
    bool mouseOverPanel = (rawMouse.second >= (float)Input::GetWindowSize().second - 130.0f);

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

        DrawGrid(proj * view, camera->Position, snappedPos);

        if (Input::IsMouseButtonJustPressed(0) && !mouseOverPanel && !m_JustSelectedFromPanel) {
            m_MovingMachineEntity = { std::numeric_limits<std::size_t>::max(), 0 };
            m_MovingGroup.clear();
        }
        m_JustSelectedFromPanel = false;
        return;
    }

    if (m_HeldMachineIndex < 0 && m_MovingMachineEntity.id == std::numeric_limits<std::size_t>::max()) {
        if (Input::IsMouseButtonJustPressed(0) && !mouseOverPanel) {
            Entity hit = Physics::GetHoveredEntity(ray, activeScene, true, true);
            if (hit.id != std::numeric_limits<std::size_t>::max()) {
                auto* nsc = activeScene->GetWorld().GetComponent<NativeScriptComponent>(hit);
                if (nsc) {
                    bool isMachine = false;
                    for (auto& s : nsc->Scripts) {
                        if (s.Name == "PotScript" || s.Name == "CuttingBoardScript" || s.Name == "MixerScript" || s.Name == "OvenScript") {
                            isMachine = true; break;
                        }
                    }

                    if (isMachine) {
                        auto* tc = activeScene->GetWorld().GetComponent<TransformComponent>(hit);
                        if (tc) {
                            m_MovingMachineOriginalPos = tc->GetPosition();
                            m_MovingMachineEntity = hit;
                            m_MovingGroup.clear();
                            glm::ivec2 machineCell = GridSystem::WorldToCell(tc->GetPosition());

                            auto* transforms = activeScene->GetWorld().GetComponentVector<TransformComponent>();
                            if (transforms) {
                                for (size_t idx = 0; idx < transforms->dense.size(); ++idx) {
                                    Entity candidate = transforms->reverse[idx];
                                    if (candidate.id == hit.id) continue;
                                    glm::vec3 candPos = transforms->dense[idx].GetPosition();
                                    if (GridSystem::WorldToCell(candPos) == machineCell) {
                                        m_MovingGroup.push_back({ candidate, candPos - tc->GetPosition() });
                                    }
                                }
                            }
                        }
                    }
                }
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

    DrawGrid(proj * view, camera->Position, snappedPos);

    if (Input::IsMouseButtonJustPressed(0) && !mouseOverPanel && !m_JustSelectedFromPanel) {
        for (auto& [ent, _] : m_PreviewGroup) world.GetEventBus().Publish(EntityDestroyRequestEvent{ ent });
        m_PreviewGroup.clear();

        std::vector<Entity> placedEntities = PrefabSerializer::Deserialize(activeScene.get(), entry.PrefabPath, snappedPos);
        for (Entity ent : placedEntities) {
            auto* tagComp = world.GetComponent<TagComponent>(ent);
            if (tagComp) tagComp->Tag = entry.Label;
        }
        m_HeldMachineIndex = -1;
    }
    m_JustSelectedFromPanel = false;
}

void BuildModePanel::DrawOverlay(float gameW, float gameH, float baseScale) {
    Renderer2D::DrawQuad({ 0.0f, 0.0f }, { gameW, gameH }, { 0.05f, 0.05f, 0.05f, 0.15f }, 0.0f);
    auto pausedTextTex = AssetManager::GetTexture("assets://UI/pausedText.png");
    if (pausedTextTex && pausedTextTex->GetRendererID() != 0) {
        float aspect = (float)pausedTextTex->GetWidth() / (float)pausedTextTex->GetHeight();
        glm::vec2 tSize = { gameW * 0.35f, (gameW * 0.35f) / aspect };
        Renderer2D::DrawQuad({ (gameW - tSize.x) * 0.5f, gameH * 0.08f }, tSize, pausedTextTex->GetRendererID(), { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    }
}

void BuildModePanel::DrawGrid(const glm::mat4& viewProj3D, const glm::vec3& camPos, const glm::vec3& hoverPos) {
    const float cell = GridSystem::CELL_SIZE;
    const float t = 0.06f;
    const float range = 30.0f;

    glm::vec4 lineColor = { 0.6f, 0.6f, 0.6f, 0.40f };
    glm::vec4 hoverColor = { 0.3f, 0.75f, 1.0f, 0.55f };

    int startX = (int)std::floor((camPos.x - range) / cell);
    int endX = (int)std::ceil((camPos.x + range) / cell);
    int startZ = (int)std::floor((camPos.z - range) / cell);
    int endZ = (int)std::ceil((camPos.z + range) / cell);

    float minX = startX * cell, maxX = endX * cell;
    float minZ = startZ * cell, maxZ = endZ * cell;
    float lenX = maxX - minX, lenZ = maxZ - minZ;
    float cX = (minX + maxX) * 0.5f;
    float cZ = (minZ + maxZ) * 0.5f;

    glm::ivec2 hCell = GridSystem::WorldToCell(hoverPos);
    glm::vec3  hCenter = { (hCell.x + 0.5f) * cell, 0.01f, (hCell.y + 0.5f) * cell };

    auto FlatQuad = [](const glm::vec3& center, float sx, float sz) -> glm::mat4 {
        return glm::translate(glm::mat4(1.0f), center) * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), { 1.f, 0.f, 0.f }) * glm::scale(glm::mat4(1.0f), { sx, sz, 1.f }) * glm::translate(glm::mat4(1.0f), { -0.5f, -0.5f, 0.f });
        };

    Renderer2D::EndScene();
    Renderer2D::BeginScene(viewProj3D);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Renderer2D::DrawQuad(FlatQuad(hCenter, cell, cell), hoverColor);
    for (int cz = startZ; cz <= endZ; ++cz) Renderer2D::DrawQuad(FlatQuad({ cX, 0.01f, cz * cell }, lenX, t), lineColor);
    for (int cx = startX; cx <= endX; ++cx) Renderer2D::DrawQuad(FlatQuad({ cx * cell, 0.01f, cZ }, t, lenZ), lineColor);

    Renderer2D::EndScene();
    glEnable(GL_DEPTH_TEST);
}