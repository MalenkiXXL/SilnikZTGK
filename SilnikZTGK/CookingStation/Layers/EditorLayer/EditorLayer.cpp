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

void EditorLayer::OnAttach() {
    // pobieramy aktualny rozmiar okna
    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = (float)windowSize.first;
    m_ViewportHeight = (float)windowSize.second;
}

void EditorLayer::OnUpdate(Timestep ts) {
    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    if (!activeScene) return;
    activeScene->CalculateTransforms();

    // Pobieramy podstawowe dane dla renderera
    auto& world = activeScene->GetWorld();
    auto* camera = activeScene->GetCamera();
    glm::vec3 camPos = camera ? camera->Position : glm::vec3(0.0f);

    float fboWidth = m_TargetFBO ? (float)m_TargetFBO->GetSpecification().Width : m_ViewportWidth;
    float fboHeight = m_TargetFBO ? (float)m_TargetFBO->GetSpecification().Height : m_ViewportHeight;
    float aspectRatio = fboWidth / (fboHeight > 0 ? fboHeight : 1.0f);

    if (camera) {
        camera->AspectRatio = aspectRatio;
    }

    float orthoSize = 10.0f;
    float worldHeight = orthoSize * 2.0f;
    float worldWidth = worldHeight * aspectRatio;

    // Używamy wymiarów FBO dla rysowania Gizmo, ponieważ renderujemy je wewnątrz tekstury gry
    glm::mat4 uiProj = glm::ortho(0.0f, fboWidth, fboHeight, 0.0f);

    glDisable(GL_DEPTH_TEST);
    Renderer2D::BeginScene(uiProj);

    if (m_SceneState == SceneState::Edit)
    {
        // ------------------ TYLKO W TRYBIE EDYCJI ------------------

        // Granice okna gry (Viewport) dokładnie takie jak w GuiLayer
        glm::vec2 viewportPos = { 200.0f, 30.0f };
        glm::vec2 viewportSize = { m_ViewportWidth - 500.0f, m_ViewportHeight - 230.0f };

        auto mousePos = Input::GetMousePosition();
        float mouseX = mousePos.first;
        float mouseY = mousePos.second;

        // BARIERA UI: Sprawdzamy, czy kursor jest wewnątrz okienka gry
        bool isMouseInViewport = (mouseX >= viewportPos.x && mouseX <= (viewportPos.x + viewportSize.x) &&
            mouseY >= viewportPos.y && mouseY <= (viewportPos.y + viewportSize.y));

        // Przeliczamy globalną pozycję myszy na pozycję lokalną wewnątrz okienka gry (gdzie lewy górny róg gry to 0,0)
        float localMouseX = mouseX - viewportPos.x;
        float localMouseY = mouseY - viewportPos.y;

        // 1. KŁADZENIE MODELI (Placement)
        auto& request = activeScene->GetPlacementRequest();
        if (request.Active) {
            m_PendingModelName = request.Name;
            m_PendingModelPath = request.Path;
            m_IsPlacing = true;
            request.Active = false;
        }

        // Stawiamy obiekt TYLKO jeśli myszka jest nad grą (isMouseInViewport)
        if (m_IsPlacing && Input::IsMouseButtonPressed(0) && isMouseInViewport)
        {
            // Obliczanie pozycji z użyciem lokalnych współrzędnych okienka gry
            float xRatio = localMouseX / viewportSize.x;
            float yRatio = localMouseY / viewportSize.y;

            glm::vec3 spawnPosition;
            spawnPosition.x = (xRatio * worldWidth - (worldWidth / 2.0f)) + camPos.x;
            spawnPosition.y = (-(yRatio * worldHeight - (worldHeight / 2.0f))) + camPos.y;
            spawnPosition.z = 0.0f;

            std::shared_ptr<Shader> modelShader = AssetManager::GetShaders().Get("ModelShader");

            std::unique_ptr<Command> cmd = std::make_unique<CreateEntityCommand>(
                &world, m_PendingModelName, m_PendingModelPath, spawnPosition, modelShader
            );

            m_CommandHistory.ExecuteCommand(std::move(cmd));

            m_IsPlacing = false;
        }

        // 2. WYBIERANIE MYSZKĄ (Raycasting)
        // Ignorujemy kliknięcia podczas stawiania obiektów oraz poza obszarem gry
        if (Input::IsMouseButtonPressed(0) && isMouseInViewport && !m_IsPlacing) {

            glm::mat4 projection = glm::ortho(-aspectRatio * orthoSize, aspectRatio * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
            glm::mat4 view = activeScene->GetCamera() ? activeScene->GetCamera()->GetViewMatrix() : glm::mat4(1.0f);

            // Wysyłamy do promienia lokalne współrzędne i rozmiary FBO, a nie całego ekranu
            Ray ray = Physics::CastRayFromMouse(localMouseX, localMouseY, viewportSize.x, viewportSize.y, projection, view);

            auto* meshStorage = world.GetComponentVector<MeshComponent>();
            auto* transformStorage = world.GetComponentVector<TransformComponent>();

            if (meshStorage != nullptr && transformStorage != nullptr) {
                for (auto it = 0; it < meshStorage->dense.size(); it++) {
                    Entity entity = meshStorage->reverse[it];
                    TransformComponent* transform = transformStorage->Get(entity);
                    if (transform != nullptr) {
                        glm::vec3 globalPos = glm::vec3(transform->WorldMatrix[3][0], transform->WorldMatrix[3][1], transform->WorldMatrix[3][2]);
                        AABB box;
                        box.Min = globalPos - transform->Scale;
                        box.Max = globalPos + transform->Scale;
                        if (Physics::Intersects(ray, box)) {
                            activeScene->SetSelectedEntity(entity);
                            break;
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
                glm::vec3 globalPos = glm::vec3(transform->WorldMatrix[3][0], transform->WorldMatrix[3][1], transform->WorldMatrix[3][2]);

                // Rysowanie względem FBO, nie całego ekranu
                float screenX = (((globalPos.x - camPos.x) + (worldWidth / 2.0f)) / worldWidth) * fboWidth;
                float screenY = ((-(globalPos.y - camPos.y) + (worldHeight / 2.0f)) / worldHeight) * fboHeight;

                Renderer2D::DrawQuad({ screenX, screenY }, { 50.0f, 2.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
                Renderer2D::DrawQuad({ screenX, screenY }, { 2.0f, 50.0f }, { 0.0f, 1.0f, 0.0f, 1.0f });
            }
        }
    }
    else
    {
        // ------------------ TYLKO W TRYBIE GRY ------------------
        activeScene->OnUpdateRuntime(ts);
    }

    Renderer2D::EndScene();
    glEnable(GL_DEPTH_TEST);

    if (m_TargetFBO) {
        m_TargetFBO->Unbind();
    }

    auto windowSize = Input::GetWindowSize();
    glViewport(0, 0, windowSize.first, windowSize.second);
}

void EditorLayer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);

    // Przechwytujemy klasę EditorLayer przez wskaźnik [this], 
    // a następnie wywołujemy odpowiednie funkcje przekazując im event.

    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
        return OnWindowResize(ev);
        });

    dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
        return OnKeyPressed(ev);
        });

    dispatcher.Dispatch<EntityTransformChangedEvent>([this](EntityTransformChangedEvent& ev) {
        return OnEntityTransformChanged(ev);
        });

    dispatcher.Dispatch<EntityDeletedEvent>([this](EntityDeletedEvent& ev) {
        return OnEntityDeleted(ev);
        });

    dispatcher.Dispatch<ScenePlayEvent>([this](ScenePlayEvent& ev) {
        return OnScenePlayEvent(ev);
        });

    dispatcher.Dispatch<SceneStopEvent>([this](SceneStopEvent& ev) {
        return OnSceneStopEvent(ev);
        });
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