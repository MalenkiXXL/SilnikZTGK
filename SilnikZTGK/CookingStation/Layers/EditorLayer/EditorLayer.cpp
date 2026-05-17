#include "EditorLayer.h"
#include <GLFW/glfw3.h>
#include "CookingStation/Core/Input.h"
#include "CookingStation/Events/KeyEvent.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include "CookingStation/Scene/ecs.h"
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Layers/GuiLayer/Gui.h" 
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Scene/SceneSerializer.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Scripts/RotationScript.h" 
#include "CreateEntityCommand.h"
#include "MoveEntityCommand.h"
#include <functional> 
#include <chrono>
#include "CookingStation/Core/GridSystem.h"
#include "SilnikZTGK/CookingStation/Scripts/ConveyorBelt/ConveyorScript.h"
#include "CookingStation/Scene/PrefabSerializer.h"

float GridSystem::CELL_SIZE = 2.0f;
static bool s_UseSSA = true;

//Helper: quad leżący płasko na podłodze(płaszczyzna XZ), środek w 'center'.
static glm::mat4 FlatQuadTransform(const glm::vec3& center, float sizeX, float sizeZ)
{
    return glm::translate(glm::mat4(1.0f), center)
        * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), { 1.0f, 0.0f, 0.0f })
        * glm::scale(glm::mat4(1.0f), { sizeX, sizeZ, 1.0f })
        * glm::translate(glm::mat4(1.0f), { -0.5f, -0.5f, 0.0f });
}

void EditorLayer::OnAttach() {
    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = (float)windowSize.first;
    m_ViewportHeight = (float)windowSize.second;
}

void EditorLayer::DrawGrid(const glm::mat4& viewProj3D, const glm::vec3& camPos, float range)
{
    const float cell = GridSystem::CELL_SIZE;
    const float t = 0.06f;

    glm::vec4 borderColor = { 0.6f, 0.6f, 0.6f, 0.55f };
    glm::vec4 hoverColor = { 0.2f, 0.9f, 0.4f, 0.50f };

    int startX = (int)std::floor((camPos.x - range) / cell);
    int endX = (int)std::ceil((camPos.x + range) / cell);
    int startZ = (int)std::floor((camPos.z - range) / cell);
    int endZ = (int)std::ceil((camPos.z + range) / cell);

    float gridMinX = startX * cell;
    float gridMaxX = endX * cell;
    float gridMinZ = startZ * cell;
    float gridMaxZ = endZ * cell;
    float gridLenX = gridMaxX - gridMinX;
    float gridLenZ = gridMaxZ - gridMinZ;
    float centerX = (gridMinX + gridMaxX) * 0.5f;
    float centerZ = (gridMinZ + gridMaxZ) * 0.5f;

    glm::ivec2 hoverCell = GridSystem::WorldToCell(m_GridHoverPos);

    Renderer2D::EndScene();
    Renderer2D::BeginScene(viewProj3D);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 1. Podświetlenie hover kafelka
    glm::vec3 hoverCenter = { (hoverCell.x + 0.5f) * cell, 0.0f, (hoverCell.y + 0.5f) * cell };
    Renderer2D::DrawQuad(FlatQuadTransform(hoverCenter, cell, cell), hoverColor);

    // 2. Linie poziome 
    for (int cz = startZ; cz <= endZ; cz++) {
        float z = cz * cell;
        Renderer2D::DrawQuad(FlatQuadTransform({ centerX, 0.0f, z }, gridLenX, t), borderColor);
    }

    // 3. Linie pionowe 
    for (int cx = startX; cx <= endX; cx++) {
        float x = cx * cell;
        Renderer2D::DrawQuad(FlatQuadTransform({ x, 0.0f, centerZ }, t, gridLenZ), borderColor);
    }

    Renderer2D::EndScene();
    glEnable(GL_DEPTH_TEST);
}

//obliczanie przeciecia promienia z podloga
void EditorLayer::UpdateGridPlacement(float localMouseX, float localMouseY, const glm::vec2& viewportSize, const glm::mat4& projection3D, const glm::mat4& view3D)
{
    Ray ray = Physics::CastRayFromMouse(localMouseX, localMouseY, viewportSize.x, viewportSize.y, projection3D, view3D);

    if (std::abs(ray.Direction.y) > 1e-6f)
    {
        float t = -ray.Origin.y / ray.Direction.y;
        if (t > 0.0f)
        {
            glm::vec3 hitPoint = ray.Origin + t * ray.Direction;
            m_GridHoverPos = GridSystem::SnapToGrid(hitPoint);
        }
    }
}

void EditorLayer::OnUpdate(Timestep ts)
{
    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    if (!activeScene) return;

    activeScene->CalculateTransforms();
    activeScene->UpdateSpatialGrid();

    auto& world = activeScene->GetWorld();
    auto* camera = activeScene->GetCamera();
    glm::vec3 camPos = camera ? camera->Position : glm::vec3(0.0f);

    float fboWidth = m_TargetFBO ? (float)m_TargetFBO->GetSpecification().Width : m_ViewportWidth;
    float fboHeight = m_TargetFBO ? (float)m_TargetFBO->GetSpecification().Height : m_ViewportHeight;
    float aspectRatio = fboWidth / (fboHeight > 0.0f ? fboHeight : 1.0f);

    if (camera) camera->AspectRatio = aspectRatio;

    float orthoSize = camera ? (10.0f * (camera->Zoom / 45.0f)) : 10.0f;

    glm::mat4 proj3D = glm::ortho(-aspectRatio * orthoSize, aspectRatio * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
    glm::mat4 view3D = camera ? camera->GetViewMatrix() : glm::mat4(1.0f);

    glm::mat4 uiProj = glm::ortho(0.0f, fboWidth, fboHeight, 0.0f);

    glDisable(GL_DEPTH_TEST);
    Renderer2D::BeginScene(uiProj);

    if (m_SceneState == SceneState::Edit)
    {
        glm::vec2 viewportPos = { 200.0f, 30.0f };
        glm::vec2 viewportSize = { m_ViewportWidth - 500.0f, m_ViewportHeight - 230.0f };

        auto mousePos = Input::GetMousePosition();
        float mouseX = mousePos.first;
        float mouseY = mousePos.second;

        bool isMouseInViewport = (
            mouseX >= viewportPos.x && mouseX <= (viewportPos.x + viewportSize.x) &&
            mouseY >= viewportPos.y && mouseY <= (viewportPos.y + viewportSize.y));

        float localMouseX = mouseX - viewportPos.x;
        float localMouseY = mouseY - viewportPos.y;

        auto& gridReq = activeScene->GetGridRequest();

        auto& request = activeScene->GetPlacementRequest();
        if (request.Active)
        {
            m_PendingModelName = request.Name;
            m_PendingModelPath = request.Path;
            m_IsPlacing = true;
            request.Active = false;
        }

        if (m_IsPlacing && Input::IsMouseButtonPressed(0) && isMouseInViewport)
        {
            Ray ray = Physics::CastRayFromMouse(mouseX, mouseY, fboWidth, fboHeight, proj3D, view3D);
            glm::vec3 spawnPosition = glm::vec3(0.0f);

            if (std::abs(ray.Direction.y) > 1e-6f) {
                float t = -ray.Origin.y / ray.Direction.y;
                if (t > 0.0f) {
                    spawnPosition = ray.Origin + t * ray.Direction;
                }
            }

            if (gridReq.Active) {
                spawnPosition = GridSystem::SnapToGrid(spawnPosition);
            }

            if (m_PendingModelPath.find(".json") != std::string::npos) {
                PrefabSerializer::Deserialize(activeScene.get(), m_PendingModelPath, spawnPosition);
                spdlog::info("EditorLayer: Postawiono prefab z pliku: {}", m_PendingModelPath);
            }
            else {
                std::unique_ptr<Command> cmd = std::make_unique<CreateEntityCommand>(
                    &world, m_PendingModelName, m_PendingModelPath, spawnPosition);
                m_CommandHistory.ExecuteCommand(std::move(cmd));
            }

            m_IsPlacing = false;
        }

        Entity selected = activeScene->GetSelectedEntity();
        bool validEntity = (selected.id != std::numeric_limits<std::size_t>::max());

        if (gridReq.Active && validEntity)
        {
            if (isMouseInViewport)
            {
                UpdateGridPlacement(localMouseX, localMouseY, viewportSize, proj3D, view3D);
            }

            DrawGrid(proj3D * view3D, camPos, 40.0f);
            Renderer2D::BeginScene(uiProj);

            if (Input::IsMouseButtonJustPressed(0) && isMouseInViewport)
            {
                auto* transform = world.GetComponent<TransformComponent>(selected);
                if (transform)
                {
                    glm::vec3 oldPos = transform->GetPosition();
                    glm::vec3 newPos = { m_GridHoverPos.x, oldPos.y, m_GridHoverPos.z };

                    std::unique_ptr<Command> cmd = std::make_unique<MoveEntityCommand>(
                        &world, selected, oldPos, newPos);
                    m_CommandHistory.ExecuteCommand(std::move(cmd));

                    spdlog::info("Grid: Przeniesiono encje ID:{} na kafelek", selected.id);
                }
                gridReq.Active = false;
            }
        }

        if (Input::IsMouseButtonJustPressed(0) && isMouseInViewport && !m_IsPlacing && !gridReq.Active)
        {
            Ray ray = Physics::CastRayFromMouse(localMouseX, localMouseY, viewportSize.x, viewportSize.y, proj3D, view3D);

            // Zmiana: pobieramy Component Storage colliderów a nie meshy
            auto* colliderStorage = world.GetComponentVector<BoxColliderComponent>();
            auto* transformStorage = world.GetComponentVector<TransformComponent>();

            auto start = std::chrono::high_resolution_clock::now();

            Entity closestEntity = { std::numeric_limits<std::size_t>::max(), 0 };
            float closestDist = std::numeric_limits<float>::max();

            if (s_UseSSA)
            {
                if (colliderStorage && transformStorage && std::abs(ray.Direction.y) > 1e-6f)
                {
                    float yMax = 15.0f;
                    float yMin = -5.0f;

                    float tTop = (yMax - ray.Origin.y) / ray.Direction.y;
                    float tBottom = (yMin - ray.Origin.y) / ray.Direction.y;

                    if (tTop > tBottom) std::swap(tTop, tBottom);

                    if (tBottom > 0.0f)
                    {
                        if (tTop < 0.0f) tTop = 0.0f;

                        glm::vec3 hitTop = ray.Origin + tTop * ray.Direction;
                        glm::vec3 hitBottom = ray.Origin + tBottom * ray.Direction;

                        glm::ivec2 cellTop = GridSystem::WorldToCell(hitTop);
                        glm::ivec2 cellBottom = GridSystem::WorldToCell(hitBottom);

                        int minX = std::min(cellTop.x, cellBottom.x) - 1;
                        int maxX = std::max(cellTop.x, cellBottom.x) + 1;
                        int minZ = std::min(cellTop.y, cellBottom.y) - 1;
                        int maxZ = std::max(cellTop.y, cellBottom.y) + 1;

                        for (int cx = minX; cx <= maxX; cx++)
                        {
                            for (int cz = minZ; cz <= maxZ; cz++)
                            {
                                glm::ivec2 currentCell = { cx, cz };
                                const auto* entitiesInCell = activeScene->GetEntitiesInCell(currentCell);

                                if (entitiesInCell)
                                {
                                    for (Entity entity : *entitiesInCell)
                                    {
                                        BoxColliderComponent* collider = colliderStorage->Get(entity);
                                        if (!collider) continue; // Decoupled od Mesha

                                        TransformComponent* transform = transformStorage->Get(entity);
                                        if (transform)
                                        {
                                            glm::vec3 globalPos = { transform->WorldMatrix[3][0], transform->WorldMatrix[3][1], transform->WorldMatrix[3][2] };
                                            glm::vec3 center = globalPos + collider->Offset;
                                            glm::vec3 extents = transform->GetScale() * collider->Size;

                                            glm::vec3 rot = transform->GetRotation();
                                            glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), glm::radians(rot.x), { 1, 0, 0 })
                                                * glm::rotate(glm::mat4(1.0f), glm::radians(rot.y), { 0, 1, 0 })
                                                * glm::rotate(glm::mat4(1.0f), glm::radians(rot.z), { 0, 0, 1 });

                                            // Precyzyjne przecięcie Ray-OBB
                                            glm::mat4 obbTransform = glm::translate(glm::mat4(1.0f), center) * rotMat;
                                            glm::mat4 invTransform = glm::inverse(obbTransform);

                                            Ray localRay;
                                            localRay.Origin = glm::vec3(invTransform * glm::vec4(ray.Origin, 1.0f));
                                            localRay.Direction = glm::normalize(glm::vec3(invTransform * glm::vec4(ray.Direction, 0.0f)));

                                            AABB localAABB;
                                            localAABB.center = glm::vec3(0.0f);
                                            localAABB.extents = extents;

                                            if (Physics::Intersects(localRay, localAABB))
                                            {
                                                float dist = glm::dot(center - ray.Origin, ray.Direction);
                                                if (dist < closestDist)
                                                {
                                                    closestDist = dist;
                                                    closestEntity = entity;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else // Brute-Force Raycast OBB
            {
                if (colliderStorage && transformStorage)
                {
                    for (size_t it = 0; it < colliderStorage->dense.size(); it++)
                    {
                        Entity entity = colliderStorage->reverse[it];
                        BoxColliderComponent* collider = &colliderStorage->dense[it];
                        TransformComponent* transform = transformStorage->Get(entity);

                        if (transform)
                        {
                            glm::vec3 globalPos = { transform->WorldMatrix[3][0], transform->WorldMatrix[3][1], transform->WorldMatrix[3][2] };
                            glm::vec3 center = globalPos + collider->Offset;
                            glm::vec3 extents = transform->GetScale() * collider->Size;

                            glm::vec3 rot = transform->GetRotation();
                            glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), glm::radians(rot.x), { 1, 0, 0 })
                                * glm::rotate(glm::mat4(1.0f), glm::radians(rot.y), { 0, 1, 0 })
                                * glm::rotate(glm::mat4(1.0f), glm::radians(rot.z), { 0, 0, 1 });

                            glm::mat4 obbTransform = glm::translate(glm::mat4(1.0f), center) * rotMat;
                            glm::mat4 invTransform = glm::inverse(obbTransform);

                            Ray localRay;
                            localRay.Origin = glm::vec3(invTransform * glm::vec4(ray.Origin, 1.0f));
                            localRay.Direction = glm::normalize(glm::vec3(invTransform * glm::vec4(ray.Direction, 0.0f)));

                            AABB localAABB;
                            localAABB.center = glm::vec3(0.0f);
                            localAABB.extents = extents;

                            if (Physics::Intersects(localRay, localAABB))
                            {
                                float dist = glm::dot(center - ray.Origin, ray.Direction);
                                if (dist < closestDist)
                                {
                                    closestDist = dist;
                                    closestEntity = entity;
                                }
                            }
                        }
                    }
                }
            }

            if (closestEntity.id != std::numeric_limits<std::size_t>::max())
            {
                activeScene->SetSelectedEntity(closestEntity);
            }

            auto end = std::chrono::high_resolution_clock::now();
            float timeMs = std::chrono::duration<float, std::milli>(end - start).count();

            spdlog::info("Wyszukiwanie obiektu (SSA: {}): {:.4f} ms", s_UseSSA ? "ON" : "OFF", timeMs);
        }

        if (validEntity)
        {
            auto* transform = world.GetComponent<TransformComponent>(selected);
            auto* collider = world.GetComponent<BoxColliderComponent>(selected);

            if (transform && collider)
            {
                Renderer2D::EndScene();
                Renderer2D::BeginScene(proj3D * view3D);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDisable(GL_DEPTH_TEST);

                glm::vec3 globalPos = { transform->WorldMatrix[3][0], transform->WorldMatrix[3][1], transform->WorldMatrix[3][2] };
                glm::vec3 center = globalPos + collider->Offset;
                glm::vec3 e = transform->GetScale() * collider->Size;

                glm::vec3 currentRot = transform->GetRotation();
                glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(currentRot.x), { 1, 0, 0 })
                    * glm::rotate(glm::mat4(1.0f), glm::radians(currentRot.y), { 0, 1, 0 })
                    * glm::rotate(glm::mat4(1.0f), glm::radians(currentRot.z), { 0, 0, 1 });

                // Macierz dla renderowania Quadów
                glm::mat4 obbTransform = glm::translate(glm::mat4(1.0f), center) * rot;

                float t = 0.05f;
                float h = t / 2.0f;
                glm::vec4 color = { 0.2f, 0.9f, 0.2f, 0.8f };

                Renderer2D::DrawQuad(obbTransform * glm::translate(glm::mat4(1.0f), { -e.x,  e.y - h,  e.z }) * glm::scale(glm::mat4(1.0f), { e.x * 2.0f, t, 1.0f }), color);
                Renderer2D::DrawQuad(obbTransform * glm::translate(glm::mat4(1.0f), { -e.x, -e.y - h,  e.z }) * glm::scale(glm::mat4(1.0f), { e.x * 2.0f, t, 1.0f }), color);
                Renderer2D::DrawQuad(obbTransform * glm::translate(glm::mat4(1.0f), { -e.x,  e.y - h, -e.z }) * glm::scale(glm::mat4(1.0f), { e.x * 2.0f, t, 1.0f }), color);
                Renderer2D::DrawQuad(obbTransform * glm::translate(glm::mat4(1.0f), { -e.x, -e.y - h, -e.z }) * glm::scale(glm::mat4(1.0f), { e.x * 2.0f, t, 1.0f }), color);

                Renderer2D::DrawQuad(obbTransform * glm::translate(glm::mat4(1.0f), { e.x - h, -e.y,  e.z }) * glm::scale(glm::mat4(1.0f), { t, e.y * 2.0f, 1.0f }), color);
                Renderer2D::DrawQuad(obbTransform * glm::translate(glm::mat4(1.0f), { -e.x - h, -e.y,  e.z }) * glm::scale(glm::mat4(1.0f), { t, e.y * 2.0f, 1.0f }), color);
                Renderer2D::DrawQuad(obbTransform * glm::translate(glm::mat4(1.0f), { e.x - h, -e.y, -e.z }) * glm::scale(glm::mat4(1.0f), { t, e.y * 2.0f, 1.0f }), color);
                Renderer2D::DrawQuad(obbTransform * glm::translate(glm::mat4(1.0f), { -e.x - h, -e.y, -e.z }) * glm::scale(glm::mat4(1.0f), { t, e.y * 2.0f, 1.0f }), color);

                glm::mat4 rotZ = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), { 0.0f, 1.0f, 0.0f });
                Renderer2D::DrawQuad(obbTransform * glm::translate(glm::mat4(1.0f), { e.x - h,  e.y - h, -e.z }) * rotZ * glm::scale(glm::mat4(1.0f), { e.z * 2.0f, t, 1.0f }), color);
                Renderer2D::DrawQuad(obbTransform * glm::translate(glm::mat4(1.0f), { -e.x - h,  e.y - h, -e.z }) * rotZ * glm::scale(glm::mat4(1.0f), { e.z * 2.0f, t, 1.0f }), color);
                Renderer2D::DrawQuad(obbTransform * glm::translate(glm::mat4(1.0f), { e.x - h, -e.y - h, -e.z }) * rotZ * glm::scale(glm::mat4(1.0f), { e.z * 2.0f, t, 1.0f }), color);
                Renderer2D::DrawQuad(obbTransform * glm::translate(glm::mat4(1.0f), { -e.x - h, -e.y - h, -e.z }) * rotZ * glm::scale(glm::mat4(1.0f), { e.z * 2.0f, t, 1.0f }), color);

                Renderer2D::EndScene();
                glEnable(GL_DEPTH_TEST);
                Renderer2D::BeginScene(uiProj);
            }

            if (transform)
            {
                glm::vec3 globalPos = { transform->WorldMatrix[3][0], transform->WorldMatrix[3][1], transform->WorldMatrix[3][2] };
                glm::vec4 clipSpacePos = proj3D * view3D * glm::vec4(globalPos, 1.0f);

                if (clipSpacePos.w != 0.0f) {
                    glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;
                    float screenX = (ndcSpacePos.x + 1.0f) * 0.5f * fboWidth;
                    float screenY = (1.0f - ndcSpacePos.y) * 0.5f * fboHeight;

                    Renderer2D::DrawQuad({ screenX, screenY }, { 50.0f, 2.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
                    Renderer2D::DrawQuad({ screenX, screenY }, { 2.0f, 50.0f }, { 0.0f, 1.0f, 0.0f, 1.0f });
                }
            }
        }
    }
    else // TRYB PLAY
    {
        activeScene->OnUpdateRuntime(ts);

        if (Input::IsMouseButtonJustPressed(0))
        {
            auto mousePos = Input::GetMousePosition();
            float localMouseX = mousePos.first - 200.0f;
            float localMouseY = mousePos.second - 30.0f;
            glm::vec2 viewportSize = { m_ViewportWidth - 500.0f, m_ViewportHeight - 230.0f };

            Ray ray = Physics::CastRayFromMouse(localMouseX, localMouseY, viewportSize.x, viewportSize.y, proj3D, view3D);

            auto& world = activeScene->GetWorld();
            // Tryb Play: pobieramy Component Storage colliderów a nie meshy
            auto* colliderStorage = world.GetComponentVector<BoxColliderComponent>();
            auto* transformStorage = world.GetComponentVector<TransformComponent>();
            auto* scriptStorage = world.GetComponentVector<NativeScriptComponent>();

            Entity closestEntity = { std::numeric_limits<std::size_t>::max(), 0 };
            float closestDist = std::numeric_limits<float>::max();

            if (s_UseSSA)
            {
                if (colliderStorage && transformStorage && std::abs(ray.Direction.y) > 1e-6f)
                {
                    float yMax = 15.0f;
                    float yMin = -5.0f;

                    float tTop = (yMax - ray.Origin.y) / ray.Direction.y;
                    float tBottom = (yMin - ray.Origin.y) / ray.Direction.y;

                    if (tTop > tBottom) std::swap(tTop, tBottom);

                    if (tBottom > 0.0f)
                    {
                        if (tTop < 0.0f) tTop = 0.0f;

                        glm::vec3 hitTop = ray.Origin + tTop * ray.Direction;
                        glm::vec3 hitBottom = ray.Origin + tBottom * ray.Direction;

                        glm::ivec2 cellTop = GridSystem::WorldToCell(hitTop);
                        glm::ivec2 cellBottom = GridSystem::WorldToCell(hitBottom);

                        int minX = std::min(cellTop.x, cellBottom.x) - 1;
                        int maxX = std::max(cellTop.x, cellBottom.x) + 1;
                        int minZ = std::min(cellTop.y, cellBottom.y) - 1;
                        int maxZ = std::max(cellTop.y, cellBottom.y) + 1;

                        for (int cx = minX; cx <= maxX; cx++)
                        {
                            for (int cz = minZ; cz <= maxZ; cz++)
                            {
                                glm::ivec2 currentCell = { cx, cz };
                                const auto* entitiesInCell = activeScene->GetEntitiesInCell(currentCell);

                                if (entitiesInCell)
                                {
                                    for (Entity entity : *entitiesInCell)
                                    {
                                        BoxColliderComponent* collider = colliderStorage->Get(entity);
                                        if (!collider) continue;

                                        TransformComponent* transform = transformStorage->Get(entity);
                                        if (transform)
                                        {
                                            glm::vec3 globalPos = { transform->WorldMatrix[3][0], transform->WorldMatrix[3][1], transform->WorldMatrix[3][2] };
                                            glm::vec3 center = globalPos + collider->Offset;
                                            glm::vec3 extents = transform->GetScale() * collider->Size;

                                            glm::vec3 rot = transform->GetRotation();
                                            glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), glm::radians(rot.x), { 1, 0, 0 })
                                                * glm::rotate(glm::mat4(1.0f), glm::radians(rot.y), { 0, 1, 0 })
                                                * glm::rotate(glm::mat4(1.0f), glm::radians(rot.z), { 0, 0, 1 });

                                            glm::mat4 obbTransform = glm::translate(glm::mat4(1.0f), center) * rotMat;
                                            glm::mat4 invTransform = glm::inverse(obbTransform);

                                            Ray localRay;
                                            localRay.Origin = glm::vec3(invTransform * glm::vec4(ray.Origin, 1.0f));
                                            localRay.Direction = glm::normalize(glm::vec3(invTransform * glm::vec4(ray.Direction, 0.0f)));

                                            AABB localAABB;
                                            localAABB.center = glm::vec3(0.0f);
                                            localAABB.extents = extents;

                                            if (Physics::Intersects(localRay, localAABB))
                                            {
                                                float dist = glm::dot(center - ray.Origin, ray.Direction);
                                                if (dist < closestDist)
                                                {
                                                    closestDist = dist;
                                                    closestEntity = entity;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                if (colliderStorage && transformStorage)
                {
                    for (size_t it = 0; it < colliderStorage->dense.size(); it++)
                    {
                        Entity entity = colliderStorage->reverse[it];
                        BoxColliderComponent* collider = &colliderStorage->dense[it];
                        TransformComponent* transform = transformStorage->Get(entity);

                        if (transform)
                        {
                            glm::vec3 globalPos = { transform->WorldMatrix[3][0], transform->WorldMatrix[3][1], transform->WorldMatrix[3][2] };
                            glm::vec3 center = globalPos + collider->Offset;
                            glm::vec3 extents = transform->GetScale() * collider->Size;

                            glm::vec3 rot = transform->GetRotation();
                            glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), glm::radians(rot.x), { 1, 0, 0 })
                                * glm::rotate(glm::mat4(1.0f), glm::radians(rot.y), { 0, 1, 0 })
                                * glm::rotate(glm::mat4(1.0f), glm::radians(rot.z), { 0, 0, 1 });

                            glm::mat4 obbTransform = glm::translate(glm::mat4(1.0f), center) * rotMat;
                            glm::mat4 invTransform = glm::inverse(obbTransform);

                            Ray localRay;
                            localRay.Origin = glm::vec3(invTransform * glm::vec4(ray.Origin, 1.0f));
                            localRay.Direction = glm::normalize(glm::vec3(invTransform * glm::vec4(ray.Direction, 0.0f)));

                            AABB localAABB;
                            localAABB.center = glm::vec3(0.0f);
                            localAABB.extents = extents;

                            if (Physics::Intersects(localRay, localAABB))
                            {
                                float dist = glm::dot(center - ray.Origin, ray.Direction);
                                if (dist < closestDist)
                                {
                                    closestDist = dist;
                                    closestEntity = entity;
                                }
                            }
                        }
                    }
                }
            }

            if (closestEntity.id != std::numeric_limits<std::size_t>::max())
            {
                if (scriptStorage)
                {
                    auto* nsc = scriptStorage->Get(closestEntity);
                    if (nsc) {
                        for (auto& s : nsc->Scripts) if (s.Instance) s.Instance->OnClick();
                    }
                }
            }
        }
    }
    Renderer2D::EndScene();
    glEnable(GL_DEPTH_TEST);

    if (m_TargetFBO)
        m_TargetFBO->Unbind();

    auto windowSize = Input::GetWindowSize();
    glViewport(0, 0, windowSize.first, windowSize.second);
}

void EditorLayer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) { return OnWindowResize(ev); });
    dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) { return OnKeyPressed(ev); });
    dispatcher.Dispatch<EntityTransformChangedEvent>([this](EntityTransformChangedEvent& ev) { return OnEntityTransformChanged(ev); });
    dispatcher.Dispatch<EntityDeletedEvent>([this](EntityDeletedEvent& ev) { return OnEntityDeleted(ev); });
    dispatcher.Dispatch<ScenePlayEvent>([this](ScenePlayEvent& ev) { return OnScenePlayEvent(ev); });
    dispatcher.Dispatch<SceneStopEvent>([this](SceneStopEvent& ev) { return OnSceneStopEvent(ev); });
}

bool EditorLayer::OnWindowResize(WindowResizeEvent& e) {
    m_ViewportWidth = (float)e.GetWidth();
    m_ViewportHeight = (float)e.GetHeight();
    return false;
}

bool EditorLayer::OnKeyPressed(KeyPressedEvent& e) {
    bool control = Input::IsKeyPressed(GLFW_KEY_LEFT_CONTROL) || Input::IsKeyPressed(GLFW_KEY_RIGHT_CONTROL);
    bool shift = Input::IsKeyPressed(GLFW_KEY_LEFT_SHIFT) || Input::IsKeyPressed(GLFW_KEY_RIGHT_SHIFT);

    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();

    if (e.GetKeyCode() == GLFW_KEY_G && m_SceneState == SceneState::Edit && activeScene)
    {
        Entity selected = activeScene->GetSelectedEntity();
        if (selected.id != std::numeric_limits<std::size_t>::max())
        {
            auto& gridReq = activeScene->GetGridRequest();
            gridReq.Active = !gridReq.Active;

            if (gridReq.Active) {
                auto* transform = activeScene->GetWorld().GetComponent<TransformComponent>(selected);
                if (transform) {
                    glm::vec3 currentPos = transform->GetPosition();
                    m_GridEntityStartPos = currentPos;
                    m_GridHoverPos = GridSystem::SnapToGrid(currentPos);
                }
                spdlog::info("Grid Mode: ON (encja ID:{})", selected.id);
            }
            else {
                spdlog::info("Grid Mode: OFF");
            }
            return true;
        }
    }

    if (e.GetKeyCode() == GLFW_KEY_ESCAPE && activeScene)
    {
        auto& gridReq = activeScene->GetGridRequest();
        if (gridReq.Active) {
            gridReq.Active = false;
            spdlog::info("Grid Mode: Anulowano");
            return true;
        }
    }

    // SSA

    // F9 Przełączanie algorytmu
    if (e.GetKeyCode() == GLFW_KEY_F9) {
        s_UseSSA = !s_UseSSA;
        spdlog::info("System SSA: {} ", s_UseSSA ? "WLACZONY (Szybki)" : "WYLACZONY (Brute-Force)");
        return true;
    }

    if (e.GetKeyCode() == GLFW_KEY_F10 && activeScene) {
        spdlog::info("Generowanie niewidzialnego stres testu");

        auto& world = activeScene->GetWorld();

        // generujemy potężną siatkę 100x100 obiektów
        for (int x = 0; x < 100; x++) {
            for (int z = 0; z < 100; z++) {
                glm::vec3 spawnPos = { x * 2.0f, 0.0f, z * 2.0f };

                Entity e = world.CreateEntity();

                world.AddComponent<TagComponent>(e, TagComponent{ "StresTest_Pudlo" });

                // 3. Dodajemy Transform
                TransformComponent trans;
                trans.SetPosition(spawnPos);
                trans.SetScale(glm::vec3(1.0f));
                world.AddComponent<TransformComponent>(e, trans);

                // ZMIANA ZWIĄZANA Z NOWĄ ARCHITEKTURĄ FIZYKI
                // Usunięto dodawanie MeshComponent z modelem nullptr!
                // Fizyka iteruje teraz po colliderach, a nie meszach, 
                // dzięki czemu ten stres test naturalnie stał się "niewidzialny".

                BoxColliderComponent bc;
                bc.Size = glm::vec3(1.0f);
                bc.Offset = glm::vec3(0.0f);
                world.AddComponent<BoxColliderComponent>(e, bc);
            }
        }

        spdlog::info("Stres test gotowy!");
        return true;
    }

    if (control) {
        if (e.GetKeyCode() == GLFW_KEY_Z) {
            if (shift) m_CommandHistory.Redo();
            else {
                m_CommandHistory.Undo();
                if (activeScene) activeScene->SetSelectedEntity({ std::numeric_limits<std::size_t>::max(), 0 });
            }
            return true;
        }
        else if (e.GetKeyCode() == GLFW_KEY_Y) {
            m_CommandHistory.Redo();
            return true;
        }
    }
    return false;
}

bool EditorLayer::OnEntityTransformChanged(EntityTransformChangedEvent& e) {
    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    if (!activeScene) return false;

    std::unique_ptr<Command> cmd = std::make_unique<MoveEntityCommand>(
        &activeScene->GetWorld(), e.GetEntity(), e.GetStartPos(), e.GetEndPos()
    );
    m_CommandHistory.ExecuteCommand(std::move(cmd));
    return true;
}

bool EditorLayer::OnEntityDeleted(EntityDeletedEvent& e) {
    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    if (!activeScene) return false;

    auto cmd = std::make_unique<DeleteEntityCommand>(&activeScene->GetWorld(), e.GetEntity());
    m_CommandHistory.ExecuteCommand(std::move(cmd));
    return true;
}

void EditorLayer::OnScenePlay() {
    m_SceneState = SceneState::Play;
    m_EditorScene = SceneManager::GetActiveScene();
    m_RuntimeScene = Scene::Copy(m_EditorScene);
    m_RuntimeScene->SetState(SceneState::Play);
    SceneManager::SetActiveScene(m_RuntimeScene);
    m_RuntimeScene->OnRuntimeStart();
}

void EditorLayer::OnSceneStop() {
    m_SceneState = SceneState::Edit;
    m_RuntimeScene->OnRuntimeStop();
    m_EditorScene->SetState(SceneState::Edit);
    SceneManager::SetActiveScene(m_EditorScene);
    m_RuntimeScene = nullptr;
}