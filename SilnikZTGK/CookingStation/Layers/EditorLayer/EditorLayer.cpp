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
#include "CookingStation/Scene/Scene.h"
#include <functional> 
#define BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

void EditorLayer::OnAttach() {
    // pobieramy aktualny rozmiar okna
    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = (float)windowSize.first;
    m_ViewportHeight = (float)windowSize.second;
}

void EditorLayer::OnUpdate(Timestep ts) {
    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    if (!activeScene) return;

    // Pobieramy podstawowe dane dla renderera
    auto& world = activeScene->GetWorld();
    auto* camera = activeScene->GetCamera();
    glm::vec3 camPos = camera ? camera->Position : glm::vec3(0.0f);

    float aspectRatio = m_ViewportWidth / (m_ViewportHeight > 0 ? m_ViewportHeight : 1.0f);
    float orthoSize = 10.0f;
    float worldHeight = orthoSize * 2.0f;
    float worldWidth = worldHeight * aspectRatio;
    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);

    glDisable(GL_DEPTH_TEST);
    Renderer2D::BeginScene(uiProj);

    // ==========================================================
    // BRAMKA STANÓW: EDYTOR VS GRA
    // ==========================================================
    if (m_SceneState == SceneState::Edit)
    {
        // ------------------ TYLKO W TRYBIE EDYCJI ------------------

      // 1. KŁADZENIE MODELI (Placement)
        auto& request = activeScene->GetPlacementRequest();
        if (request.Active) {
            m_PendingModelName = request.Name;
            m_PendingModelPath = request.Path;
            m_IsPlacing = true;
            request.Active = false;
        }

        auto mousePos = Input::GetMousePosition();
        float mouseX = mousePos.first;
        float mouseY = mousePos.second;

        if (m_IsPlacing && Input::IsMouseButtonPressed(0) && mouseX > 200.0f)
        {
            // Obliczanie pozycji
            float xRatio = mouseX / m_ViewportWidth;
            float yRatio = mouseY / m_ViewportHeight;
            glm::vec3 spawnPosition;
            spawnPosition.x = (xRatio * worldWidth - (worldWidth / 2.0f)) + camPos.x;
            spawnPosition.y = (-(yRatio * worldHeight - (worldHeight / 2.0f))) + camPos.y;
            spawnPosition.z = 0.0f;

            std::shared_ptr<Shader> modelShader = std::make_shared<Shader>(
                "CookingStation/Shaders/vsShaders/shader.vs",
                "CookingStation/Shaders/fragShaders/shader.frag"
            );

            std::unique_ptr<Command> cmd = std::make_unique<CreateEntityCommand>(
                &world, m_PendingModelName, m_PendingModelPath, spawnPosition, modelShader
            );

            m_CommandHistory.ExecuteCommand(std::move(cmd));

            m_IsPlacing = false;
        }

        // 2. WYBIERANIE MYSZKĄ (Raycasting)
        if (Input::IsMouseButtonPressed(0)) {
            glm::vec2 mPos = Gui::GetMappedMousePos();
            if (mPos.x > 200) {
                glm::mat4 projection = glm::ortho(-aspectRatio * orthoSize, aspectRatio * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
                glm::mat4 view = activeScene->GetCamera() ? activeScene->GetCamera()->GetViewMatrix() : glm::mat4(1.0f);
                Ray ray = Physics::CastRayFromMouse(Input::GetMousePosition().first, Input::GetMousePosition().second, m_ViewportWidth, m_ViewportHeight, projection, view);

                auto* meshStorage = world.GetComponentVector<MeshComponent>();
                auto* transformStorage = world.GetComponentVector<TransformComponent>();

                if (meshStorage != nullptr && transformStorage != nullptr) {
                    for (auto it = 0; it < meshStorage->dense.size(); it++) {
                        Entity entity = meshStorage->reverse[it];
                        TransformComponent* transform = transformStorage->Get(entity);
                        if (transform != nullptr) {
                            AABB box;
                            box.Min = transform->Position - transform->Scale;
                            box.Max = transform->Position + transform->Scale;
                            if (Physics::Intersects(ray, box)) {
                                activeScene->SetSelectedEntity(entity);
                                break;
                            }
                        }
                    }
                }
            }
        }

        // 3. RYSOWANIE OSI (Gizmo)
        Entity selected = activeScene->GetSelectedEntity();
        if (selected.id != std::numeric_limits<std::size_t>::max()) {
            auto* transform = world.GetComponent<TransformComponent>(selected);
            if (transform) {
                float screenX = (((transform->Position.x - camPos.x) + (worldWidth / 2.0f)) / worldWidth) * m_ViewportWidth;
                float screenY = ((-(transform->Position.y - camPos.y) + (worldHeight / 2.0f)) / worldHeight) * m_ViewportHeight;
                Renderer2D::DrawQuad({ screenX, screenY }, { 50.0f, 2.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
                Renderer2D::DrawQuad({ screenX, screenY }, { 2.0f, 50.0f }, { 0.0f, 1.0f, 0.0f, 1.0f });
            }
        }
    }
    else
    {
        // ------------------ TYLKO W TRYBIE GRY (PLAY) ------------------

        // Zamiast rysować rzeczy z edytora, wywołujemy właściwą pętlę silnika
        activeScene->OnUpdateRuntime(ts);
    }

    Renderer2D::EndScene();
    glEnable(GL_DEPTH_TEST);
}

void EditorLayer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
        return OnWindowResize(ev);
        });
    dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(EditorLayer::OnKeyPressed));
    dispatcher.Dispatch<EntityTransformChangedEvent>(BIND_EVENT_FN(EditorLayer::OnEntityTransformChanged));
    dispatcher.Dispatch<EntityDeletedEvent>(BIND_EVENT_FN(EditorLayer::OnEntityDeleted));
    dispatcher.Dispatch<ScenePlayEvent>(BIND_EVENT_FN(EditorLayer::OnScenePlayEvent));
    dispatcher.Dispatch<SceneStopEvent>(BIND_EVENT_FN(EditorLayer::OnSceneStopEvent));
}

bool EditorLayer::OnWindowResize(WindowResizeEvent& e) {
    m_ViewportWidth = (float)e.GetWidth();
    m_ViewportHeight = (float)e.GetHeight();
    return false;
}

bool EditorLayer::OnKeyPressed(KeyPressedEvent& e) {
    // Sprawdzamy klawisze modyfikujące (Ctrl i Shift)
    bool control = Input::IsKeyPressed(GLFW_KEY_LEFT_CONTROL) || Input::IsKeyPressed(GLFW_KEY_RIGHT_CONTROL);
    bool shift = Input::IsKeyPressed(GLFW_KEY_LEFT_SHIFT) || Input::IsKeyPressed(GLFW_KEY_RIGHT_SHIFT);

    if (control) {
        if (e.GetKeyCode() == GLFW_KEY_Z) {
            if (shift) {
                m_CommandHistory.Redo();
            } // Ctrl + Shift + Z
            else {
                m_CommandHistory.Undo();
                std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
                if (activeScene) {
                    // Ustawiamy na "pustą" encję (max ID)
                    activeScene->SetSelectedEntity({ std::numeric_limits<std::size_t>::max(), 0 });
                }
            }// Ctrl + Z

            return true;
        }
        else if (e.GetKeyCode() == GLFW_KEY_Y) {
            m_CommandHistory.Redo();     // Ctrl + Y
            return true;
        }
    }
    return false;
}

bool EditorLayer::OnEntityTransformChanged(EntityTransformChangedEvent& e) {
    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    if (!activeScene) return false;

    // EditorLayer ma dostęp do m_CommandHistory, więc po prostu tworzy i odpala
    std::unique_ptr<Command> cmd = std::make_unique<MoveEntityCommand>(
        &activeScene->GetWorld(),
        e.GetEntity(),
        e.GetStartPos(),
        e.GetEndPos()
    );

    m_CommandHistory.ExecuteCommand(std::move(cmd));

    spdlog::info("Edytor zapisał ruch dla encji ID: {}", e.GetEntity().id);
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

    // 1. Robimy kopię zapasową sceny edytora
    m_EditorScene = SceneManager::GetActiveScene();

    // 2. Tworzymy nową scenę do grania i kopiujemy do niej wszystko
    m_RuntimeScene = Scene::Copy(m_EditorScene);

    // Mówimy nowej scenie, że jest w trybie gry
    m_RuntimeScene->SetState(SceneState::Play);

    // 3. Ustawiamy scenę gry jako aktywną w silniku
    SceneManager::SetActiveScene(m_RuntimeScene);

    // 4. Odpalamy systemy (fizyka, skrypty)
    m_RuntimeScene->OnRuntimeStart();
}

void EditorLayer::OnSceneStop() {
    m_SceneState = SceneState::Edit;

    // 1. Zatrzymujemy systemy gry
    m_RuntimeScene->OnRuntimeStop();

    //  Upewniamy się, że scena edytora wraca do trybu edycji
    m_EditorScene->SetState(SceneState::Edit);

    // 2. Przywracamy oryginalną scenę edytora (obiekty wracają na swoje miejsca)
    SceneManager::SetActiveScene(m_EditorScene);

    // 3. Usuwamy scenę runtime z pamięci
    m_RuntimeScene = nullptr;
}