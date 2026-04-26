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
#include "CookingStation/Core/GridSystem.h"

float GridSystem::CELL_SIZE = 2.0f;

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
    const float t = 0.06f; // Zwiększyłem lekko grubość linii

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

    // WYMUSZAMY PRZEZROCZYSTOŚĆ I WIDOCZNOŚĆ!
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

void EditorLayer::UpdateGridPlacement(float localMouseX, float localMouseY, const glm::vec2& viewportSize, const glm::mat4& projection3D, const glm::mat4& view3D)
{
    // Używamy Waszego solidnego raycastingu z Physics.h!
    Ray ray = Physics::CastRayFromMouse(localMouseX, localMouseY, viewportSize.x, viewportSize.y, projection3D, view3D);

    // Przecięcie promienia z płaszczyzną podłogi (Y = 0)
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

    auto& world = activeScene->GetWorld();
    auto* camera = activeScene->GetCamera();
    glm::vec3 camPos = camera ? camera->Position : glm::vec3(0.0f);

    float fboWidth = m_TargetFBO ? (float)m_TargetFBO->GetSpecification().Width : m_ViewportWidth;
    float fboHeight = m_TargetFBO ? (float)m_TargetFBO->GetSpecification().Height : m_ViewportHeight;
    float aspectRatio = fboWidth / (fboHeight > 0.0f ? fboHeight : 1.0f);

    if (camera) camera->AspectRatio = aspectRatio;

    float orthoSize = camera ? (10.0f * (camera->Zoom / 45.0f)) : 10.0f;
    float worldHeight = orthoSize * 2.0f;
    float worldWidth = worldHeight * aspectRatio;

    // NAPRAWA MACIERZY: To gra izometryczna, używamy glm::ortho dla 3D, a nie perspective!
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

        auto& gridReq = activeScene->GetGridRequest(); // Pobranie stanu siatki

        // ── 1. ZWYKŁE KŁADZENIE MODELI ────────────────────────────────────────
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
            // ZMIANA: Strzelamy promieniem w świat 3D z myszki!
            Ray ray = Physics::CastRayFromMouse(localMouseX, localMouseY, viewportSize.x, viewportSize.y, proj3D, view3D);
            glm::vec3 spawnPosition = glm::vec3(0.0f);

            // Sprawdzamy w którym miejscu promień uderza w podłogę (oś Y = 0)
            if (std::abs(ray.Direction.y) > 1e-6f) {
                float t = -ray.Origin.y / ray.Direction.y;
                if (t > 0.0f) {
                    spawnPosition = ray.Origin + t * ray.Direction;
                }
            }

            // Jeśli siatka jest aktywna przyceluj do równych kratek
            if (gridReq.Active) {
                spawnPosition = GridSystem::SnapToGrid(spawnPosition);
            }

            std::shared_ptr<Shader> modelShader = AssetManager::GetShaders().Get("ModelShader");
            std::unique_ptr<Command> cmd = std::make_unique<CreateEntityCommand>(
                &world, m_PendingModelName, m_PendingModelPath, spawnPosition, modelShader);
            m_CommandHistory.ExecuteCommand(std::move(cmd));
            m_IsPlacing = false;
        }

        // ── 2. TRYB SIATKI DLA ZAZNACZONEGO OBIEKTU ───────────────────────────
        Entity selected = activeScene->GetSelectedEntity();
        bool validEntity = (selected.id != std::numeric_limits<std::size_t>::max());

        if (gridReq.Active && validEntity)
        {
            if (isMouseInViewport)
            {
                UpdateGridPlacement(localMouseX, localMouseY, viewportSize, proj3D, view3D);
            }

            DrawGrid(proj3D * view3D, camPos, 40.0f);

            // Po powrocie wznawiamy 2D UI
            Renderer2D::BeginScene(uiProj);

            if (Input::IsMouseButtonJustPressed(0) && isMouseInViewport)
            {
                auto* transform = world.GetComponent<TransformComponent>(selected);
                if (transform)
                {
                    glm::vec3 oldPos = transform->Position;
                    glm::vec3 newPos = { m_GridHoverPos.x, transform->Position.y, m_GridHoverPos.z };

                    std::unique_ptr<Command> cmd = std::make_unique<MoveEntityCommand>(
                        &world, selected, oldPos, newPos);
                    m_CommandHistory.ExecuteCommand(std::move(cmd));

                    spdlog::info("Grid: Przeniesiono encje ID:{} na kafelek", selected.id);
                }
                gridReq.Active = false; // wyłącza tryb po kliknięciu
            }
        }

        // ── 3. WYBIERANIE MYSZKĄ (Nie działa, gdy siatka przejmuje klik) ──────
        if (Input::IsMouseButtonPressed(0) && isMouseInViewport && !m_IsPlacing && !gridReq.Active)
        {
            Ray ray = Physics::CastRayFromMouse(localMouseX, localMouseY, viewportSize.x, viewportSize.y, proj3D, view3D);

            auto* meshStorage = world.GetComponentVector<MeshComponent>();
            auto* transformStorage = world.GetComponentVector<TransformComponent>();

            if (meshStorage && transformStorage)
            {
                for (size_t it = 0; it < meshStorage->dense.size(); it++)
                {
                    Entity entity = meshStorage->reverse[it];
                    TransformComponent* transform = transformStorage->Get(entity);
                    if (transform)
                    {
                        glm::vec3 globalPos = { transform->WorldMatrix[3][0], transform->WorldMatrix[3][1], transform->WorldMatrix[3][2] };
                        AABB box;
                        box.Min = globalPos - transform->Scale;
                        box.Max = globalPos + transform->Scale;
                        if (Physics::Intersects(ray, box))
                        {
                            activeScene->SetSelectedEntity(entity);
                            break;
                        }
                    }
                }
            }
        }

        // ── 4. GIZMO ─────────────────────────────────────────────────────────
        if (validEntity)
        {
            auto* transform = world.GetComponent<TransformComponent>(selected);
            if (transform)
            {
                glm::vec3 globalPos = { transform->WorldMatrix[3][0], transform->WorldMatrix[3][1], transform->WorldMatrix[3][2] };

                // ZMIANA: Prawdziwe rzutowanie matematyczne punktu 3D do okna 2D!
                glm::vec4 clipSpacePos = proj3D * view3D * glm::vec4(globalPos, 1.0f);

                if (clipSpacePos.w != 0.0f) {
                    glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;
                    float screenX = (ndcSpacePos.x + 1.0f) * 0.5f * fboWidth;
                    float screenY = (1.0f - ndcSpacePos.y) * 0.5f * fboHeight;

                    // Oś X (Czerwona) i Oś Y (Zielona) rysowane na ekranie
                    Renderer2D::DrawQuad({ screenX, screenY }, { 50.0f, 2.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
                    Renderer2D::DrawQuad({ screenX, screenY }, { 2.0f, 50.0f }, { 0.0f, 1.0f, 0.0f, 1.0f });
                }
            }
        }
    }
    else
    {
        activeScene->OnUpdateRuntime(ts);
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

    // KLAWISZ G - SIATKA
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
                    m_GridEntityStartPos = transform->Position;
                    m_GridHoverPos = GridSystem::SnapToGrid(transform->Position);
                }
                spdlog::info("Grid Mode: ON (encja ID:{})", selected.id);
            }
            else {
                spdlog::info("Grid Mode: OFF");
            }
            return true;
        }
    }

    // ESCAPE - ANULOWANIE SIATKI
    if (e.GetKeyCode() == GLFW_KEY_ESCAPE && activeScene)
    {
        auto& gridReq = activeScene->GetGridRequest();
        if (gridReq.Active) {
            gridReq.Active = false;
            spdlog::info("Grid Mode: Anulowano");
            return true;
        }
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