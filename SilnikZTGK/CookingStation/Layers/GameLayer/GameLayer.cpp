#include "GameLayer.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include "CookingStation/Scene/ScriptableEntity.h"
#include <GLFW/glfw3.h>
#include "CookingStation/Events/GameEvents.h" // DODANE: Wymagane dla EntityClickedEvent
#include <spdlog/spdlog.h> 
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void GameLayer::OnAttach()
{
    m_ActiveScene = SceneManager::GetActiveScene();

    if (!m_ActiveScene)
    {
        spdlog::error("GameLayer: Brak aktywnej sceny w SceneManager!");
        return;
    }
}

void GameLayer::OnDetach()
{
    if (m_ActiveScene)
        m_ActiveScene->OnRuntimeStop();
}


void GameLayer::OnUpdate(Timestep ts)
{
    m_ActiveScene = SceneManager::GetActiveScene();
    if (!m_ActiveScene) return;
    if (m_ActiveScene->GetState() != SceneState::Play) return;

    m_ActiveScene->OnUpdateRuntime(ts);
    auto& world = m_ActiveScene->GetWorld();

    auto mousePos = Input::GetMousePosition();
    float mouseX = mousePos.first;
    float mouseY = mousePos.second;
    auto windowSize = Input::GetWindowSize();
    float viewWidth = (float)windowSize.first;
    float viewHeight = (float)windowSize.second;

#ifndef CS_DISTRIBUTION
    mouseX -= 200.0f;
    mouseY -= 30.0f;
    viewWidth -= 500.0f;
    viewHeight -= 230.0f;
#endif

    auto* camera = m_ActiveScene->GetCamera();
    if (camera)
    {
        float aspectRatio = viewWidth / (viewHeight > 0.0f ? viewHeight : 1.0f);
        camera->AspectRatio = aspectRatio;
        float orthoSize = 10.0f * (camera->Zoom / 45.0f);
        glm::mat4 proj3D = glm::ortho(-aspectRatio * orthoSize, aspectRatio * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
        glm::mat4 view3D = camera->GetViewMatrix();

        Ray interactionRay;
        bool isClickAction = false;

        // 1. Sprawdzamy wejście z pada
        if (Input::IsGamepadPresent(0))
        {
            float leftAxisX = Input::GetGamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_X, 0);
            float leftAxisY = Input::GetGamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_Y, 0);

            // Deadzone, żeby gałka nie "pływała" sama
            if (std::abs(leftAxisX) > 0.15f || std::abs(leftAxisY) > 0.15f)
            {
                m_UsingGamepad = true;

                glm::vec3 flatForward = camera->GetFlatForward();
                glm::vec3 right = camera->Right;

                // Odwracamy Y, bo wychylenie w górę daje wartość ujemną w GLFW
                glm::vec3 moveDir = right * leftAxisX - flatForward * leftAxisY;

                if (glm::length(moveDir) > 0.1f)
                {
                    moveDir = glm::normalize(moveDir);
                    // Mnożymy przez czas klatki dla płynności
                    m_VirtualCursorPos += moveDir * m_GamepadCursorSpeed * (float)ts;

                    // Zabezpieczenie: upewnijmy się, że kursor płasko trzyma się poziomu podłogi
                    m_VirtualCursorPos.y = 0.0f;

                    // Logujemy pozycję wyliczoną przed snapowaniem do kratki
                    spdlog::info("Wirtualny kursor 3D porusza sie. Pozycja: X:{:.2f}, Z:{:.2f}", m_VirtualCursorPos.x, m_VirtualCursorPos.z);
                }
            }

            if (m_UsingGamepad && Input::IsGamepadButtonJustPressed(GLFW_GAMEPAD_BUTTON_A, 0))
            {
                isClickAction = true;
            }
        }

        // 2. Automatyczny powrót na mysz po jej poruszeniu lub kliknięciu
        static std::pair<float, float> s_LastMousePos = mousePos;
        if (std::abs(mouseX - s_LastMousePos.first) > 1.0f || std::abs(mouseY - s_LastMousePos.second) > 1.0f || Input::IsMouseButtonJustPressed(0))
        {
            m_UsingGamepad = false;
            s_LastMousePos = { mouseX, mouseY };
        }

        // 3. Budujemy finalny wektor interakcji (Ray) w zależności od kontrolera
        if (m_UsingGamepad)
        {
            // Rzutujemy kursor idealnie do srodka najbliższego kafelka (Grid)
            glm::vec3 snappedCursor = GridSystem::SnapToGrid(m_VirtualCursorPos);

            // Strzelamy laserem z góry w dół idealnie na wybrany kafelek, aby oszukać PhysicsSystem
            interactionRay.Origin = snappedCursor + glm::vec3(0.0f, 20.0f, 0.0f);
            interactionRay.Direction = glm::vec3(0.0f, -1.0f, 0.0f);
        }
        else
        {
            // Standardowa myszka
            interactionRay = Physics::CastRayFromMouse(mouseX, mouseY, viewWidth, viewHeight, proj3D, view3D);
            isClickAction = Input::IsMouseButtonJustPressed(0);

            // Aktualizujemy pozycję wirtualnego kursora do miejsca myszy, 
            // żeby po złapaniu za pada kursor nie odskoczył w inne miejsce
            if (std::abs(interactionRay.Direction.y) > 1e-6f) {
                float t = -interactionRay.Origin.y / interactionRay.Direction.y;
                if (t > 0.0f) {
                    m_VirtualCursorPos = interactionRay.Origin + t * interactionRay.Direction;
                }
            }
        }

        // 4. Publikacja eventów wykorzystująca Twój istniejący system
        Entity hoveredEntity = Physics::GetHoveredEntity(interactionRay, m_ActiveScene, true, true);
        world.GetEventBus().Publish(EntityHoveredEvent{ hoveredEntity });

        if (isClickAction)
        {
            if (hoveredEntity.id != std::numeric_limits<std::size_t>::max())
                world.GetEventBus().Publish(EntityClickedEvent{ hoveredEntity, 0 });
        }

        //Ray ray = Physics::CastRayFromMouse(mouseX, mouseY, viewWidth, viewHeight, proj3D, view3D);

        //// HOVER — co klatkę
        //Entity hoveredEntity = Physics::GetHoveredEntity(ray, m_ActiveScene, true, true);
        //world.GetEventBus().Publish(EntityHoveredEvent{ hoveredEntity });

        // KLIK — tylko przy naciśnięciu
        /*if (Input::IsMouseButtonJustPressed(0))
        {
            if (hoveredEntity.id != std::numeric_limits<std::size_t>::max())
                world.GetEventBus().Publish(EntityClickedEvent{ hoveredEntity, 0 });
        }*/

        // --- RYSOWANIE WIRTUALNEGO KURSORA DLA PADA ---
        if (m_UsingGamepad)
        {
            // 1. Jeśli encja kursora jeszcze nie istnieje w ECS (lub została usunięta), tworzymy ją
            if (m_GamepadCursor.id == std::numeric_limits<std::size_t>::max() || !world.GetComponent<TagComponent>(m_GamepadCursor))
            {
                m_GamepadCursor = world.CreateEntity();
                world.AddComponent<TagComponent>(m_GamepadCursor, TagComponent{ "VirtualCursor" });

                TransformComponent tc;
                // Spłaszczamy go na osi Y i rozszerzamy, by przypominał duży dysk podświetlający kafelek
                tc.SetScale(glm::vec3(0.07f, 0.01f, 0.07f));
                world.AddComponent<TransformComponent>(m_GamepadCursor, tc);

                MeshComponent mc;
                // Używamy talerza jako tymczasowego modelu celownika
                mc.ModelPtr = AssetManager::GetModel("CookingStation/Assets/models/wystroj/podlogakursormoje.gltf");
                world.AddComponent<MeshComponent>(m_GamepadCursor, mc);

                spdlog::info("Wirtualny Kursor: Utworzono encję z modelem talerza!");
            }

            // 2. Przenosimy kursor fizycznie na środek aktualnego kafelka
            auto* tc = world.GetComponent<TransformComponent>(m_GamepadCursor);
            if (tc)
            {
                glm::vec3 snappedCursor = GridSystem::SnapToGrid(m_VirtualCursorPos);
                snappedCursor.y += 0.1f; // Unosimy minimalnie nad podłogę
                tc->SetPosition(snappedCursor);
            }

            // --- 3. ŚLEDZENIE KAMERY (NOWE) ---
            // 1. Znajdujemy dokładny punkt na podłodze (Y=0), na który patrzy środek kamery
            glm::vec3 screenCenterOnFloor = camera->TargetPosition;
            if (std::abs(camera->Front.y) > 0.001f) // Zabezpieczenie przed kamerą patrzącą idealnie prosto
            {
                // Równanie promienia: P = Origin + t * Direction. Szukamy t dla P.y = 0
                float t = -camera->TargetPosition.y / camera->Front.y;
                screenCenterOnFloor = camera->TargetPosition + t * camera->Front;
            }

            // 2. Obliczamy różnicę pozycji między wirtualnym kursorem a rzeczywistym środkiem ekranu
            glm::vec3 diff = m_VirtualCursorPos - screenCenterOnFloor;
            diff.y = 0.0f; // Poruszamy się tylko po płaskiej podłodze

            float distance = glm::length(diff);

            // Strefa, w której kursor porusza się swobodnie bez ruszania kamery
            // (Jeśli 8.0f to dla Ciebie za dużo i kamera reaguje za późno, zmniejsz np. na 5.0f)
            float deadzone = 8.0f;

            if (distance > deadzone)
            {
                glm::vec3 moveDir = glm::normalize(diff);
                // Przesuwamy docelową pozycję kamery 
                camera->TargetPosition += moveDir * (distance - deadzone);
            }
        }
    }
}

void GameLayer::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);

    dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& event) {
        return OnKeyPressed(event);
        });
}

bool GameLayer::OnKeyPressed(KeyPressedEvent& e)
{
    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();

    if (!activeScene || activeScene->GetState() != SceneState::Play)
    {
        return false;
    }

    if (e.GetKeyCode() == 32 && e.GetRepeatCode() == 0) // Spacja
    {
        AudioEngine::Play("CookingStation/Assets/sounds/onion_chopping.mp3");
        return false;
    }

    return false;
}