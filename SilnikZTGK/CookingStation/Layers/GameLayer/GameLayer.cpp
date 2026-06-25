#include "GameLayer.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Core/Application.h"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h> 
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

    AudioEngine::PlayMusic("assets://sounds/aktasok-ambient-background-loop.mp3", true, 0.01f);

    auto& appBus = Application::Get().GetEventBus();
    m_PauseSubId = appBus.Subscribe<GamePausedEvent>([this](const GamePausedEvent&) { m_IsPaused = true; });
    m_ResumeSubId = appBus.Subscribe<GameResumedEvent>([this](const GameResumedEvent&) { m_IsPaused = false; });

    m_BuildModeSubId = appBus.Subscribe<BuildModeToggledEvent>([this](const BuildModeToggledEvent& e) {
        m_IsBuildModeActive = e.IsActive;
        });

    m_GameStartedSubId = appBus.Subscribe<GameStartedEvent>([this](const GameStartedEvent&) {
        m_IsPaused = false;
        m_IsBuildModeActive = false;
        m_IsLevelCompleted = false;

        m_ActiveScene = SceneManager::GetActiveScene();
        if (m_ActiveScene) {
            m_ActiveScene->SetState(SceneState::Play);
        }
        });
}

void GameLayer::OnDetach()
{
    if (m_ActiveScene)
        m_ActiveScene->OnRuntimeStop();

    auto& appBus = Application::Get().GetEventBus();
    if (m_PauseSubId != 0) appBus.Unsubscribe<GamePausedEvent>(m_PauseSubId);
    if (m_ResumeSubId != 0) appBus.Unsubscribe<GameResumedEvent>(m_ResumeSubId);
    if (m_BuildModeSubId != 0) appBus.Unsubscribe<BuildModeToggledEvent>(m_BuildModeSubId);
    if (m_GameStartedSubId != 0) appBus.Unsubscribe<GameStartedEvent>(m_GameStartedSubId);
}

void GameLayer::OnUpdate(Timestep ts)
{
    bool speedUp = Input::IsKeyPressed(GLFW_KEY_X);

    if (GameManagerScript::s_SpeedUpUIHeld) {
        speedUp = true;
        GameManagerScript::s_SpeedUpUIHeld = false; 
    }

    if (speedUp) {
        m_TimeScale = 4.0f;
    }
    else if (Input::IsKeyPressed(GLFW_KEY_Z)) {
        m_TimeScale = 8.0f;
    }
    else {
        m_TimeScale = 1.0f;
    }

    Timestep scaledTs = ts.GetSeconds() * m_TimeScale;
    m_ActiveScene = SceneManager::GetActiveScene();
    if (!m_ActiveScene) return;

    if (m_ActiveScene != m_LastSubscribedScene)
    {
        SubscribeToGameplayEvents(m_ActiveScene);
        m_LastSubscribedScene = m_ActiveScene;
    }

    if (m_ActiveScene->GetState() != SceneState::Play) return;

    if (m_IsPaused || m_IsBuildModeActive || m_IsLevelCompleted) return;

    m_ActiveScene->OnUpdateRuntime(scaledTs);
    auto& world = m_ActiveScene->GetWorld();

    UpdateTransformAnimations(world, scaledTs);

    if (GameManagerScript::s_IsCutscenePlaying) return;

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

        if (Input::IsGamepadPresent(0))
        {
            float leftAxisX = Input::GetGamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_X, 0);
            float leftAxisY = Input::GetGamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_Y, 0);

            if (std::abs(leftAxisX) > 0.15f || std::abs(leftAxisY) > 0.15f)
            {
                m_UsingGamepad = true;

                glm::vec3 flatForward = camera->GetFlatForward();
                glm::vec3 right = camera->Right;

                glm::vec3 moveDir = right * leftAxisX - flatForward * leftAxisY;

                if (glm::length(moveDir) > 0.1f)
                {
                    moveDir = glm::normalize(moveDir);
                    m_VirtualCursorPos += moveDir * m_GamepadCursorSpeed * (float)ts;

                    m_VirtualCursorPos.y = 0.0f;

                    spdlog::info("Wirtualny kursor 3D porusza sie. Pozycja: X:{:.2f}, Z:{:.2f}", m_VirtualCursorPos.x, m_VirtualCursorPos.z);
                }
            }

            if (m_UsingGamepad && Input::IsGamepadButtonJustPressed(GLFW_GAMEPAD_BUTTON_A, 0))
            {
                isClickAction = true;
            }
        }

        static std::pair<float, float> s_LastMousePos = mousePos;
        if (std::abs(mouseX - s_LastMousePos.first) > 1.0f || std::abs(mouseY - s_LastMousePos.second) > 1.0f || Input::IsMouseButtonJustPressed(0))
        {
            m_UsingGamepad = false;
            s_LastMousePos = { mouseX, mouseY };
        }

        if (m_UsingGamepad)
        {
            glm::vec3 snappedCursor = GridSystem::SnapToGrid(m_VirtualCursorPos);

            interactionRay.Origin = snappedCursor + glm::vec3(0.0f, 20.0f, 0.0f);
            interactionRay.Direction = glm::vec3(0.0f, -1.0f, 0.0f);
        }
        else
        {
            interactionRay = Physics::CastRayFromMouse(mouseX, mouseY, viewWidth, viewHeight, proj3D, view3D);
            isClickAction = Input::IsMouseButtonJustPressed(0);

            if (std::abs(interactionRay.Direction.y) > 1e-6f) {
                float t = -interactionRay.Origin.y / interactionRay.Direction.y;
                if (t > 0.0f) {
                    m_VirtualCursorPos = interactionRay.Origin + t * interactionRay.Direction;
                }
            }
        }

        Entity hoveredEntity = Physics::GetHoveredEntity(interactionRay, m_ActiveScene, true, true, true);
        world.GetEventBus().Publish(EntityHoveredEvent{ hoveredEntity });

        if (isClickAction)
        {
            if (hoveredEntity.id != std::numeric_limits<std::size_t>::max())
                world.GetEventBus().Publish(EntityClickedEvent{ hoveredEntity, 0 });
        }

        if (m_UsingGamepad)
        {
            if (m_GamepadCursor.id == std::numeric_limits<std::size_t>::max() || !world.GetComponent<TagComponent>(m_GamepadCursor))
            {
                m_GamepadCursor = world.CreateEntity();
                world.AddComponent<TagComponent>(m_GamepadCursor, TagComponent{ "VirtualCursor" });

                TransformComponent tc;
                tc.SetScale(glm::vec3(0.07f, 0.01f, 0.07f));
                world.AddComponent<TransformComponent>(m_GamepadCursor, tc);

                MeshComponent mc;
                mc.ModelPtr = AssetManager::GetModel("assets://models/wystroj/podlogakursormoje.gltf");
                world.AddComponent<MeshComponent>(m_GamepadCursor, mc);

                spdlog::info("Wirtualny Kursor: Utworzono encję z modelem talerza!");
            }

            auto* tc = world.GetComponent<TransformComponent>(m_GamepadCursor);
            if (tc)
            {
                glm::vec3 snappedCursor = GridSystem::SnapToGrid(m_VirtualCursorPos);
                snappedCursor.y += 0.1f;
                tc->SetPosition(snappedCursor);
            }

            glm::vec3 screenCenterOnFloor = camera->TargetPosition;
            if (std::abs(camera->Front.y) > 0.001f)
            {
                float t = -camera->TargetPosition.y / camera->Front.y;
                screenCenterOnFloor = camera->TargetPosition + t * camera->Front;
            }

            glm::vec3 diff = m_VirtualCursorPos - screenCenterOnFloor;
            diff.y = 0.0f;

            float distance = glm::length(diff);
            float deadzone = 8.0f;

            if (distance > deadzone)
            {
                glm::vec3 moveDir = glm::normalize(diff);
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

    return false;
}

void GameLayer::SubscribeToGameplayEvents(std::shared_ptr<Scene> scene)
{
    if (!scene) return;
    auto& eventBus = scene->GetWorld().GetEventBus();

    spdlog::info("AudioEngine: Podpinam pelna liste eventow audio!");

    //UI
    eventBus.Subscribe<PlayPauseSoundEvent>([](const PlayPauseSoundEvent& e) {
        AudioEngine::Play("assets://sounds/pause.mp3");
        });

    eventBus.Subscribe<PlayUnpauseSoundEvent>([](const PlayUnpauseSoundEvent& e) {
        AudioEngine::Play("assets://sounds/unpause.mp3");
        });

    eventBus.Subscribe<MachinePickedUpEvent>([](const MachinePickedUpEvent& e) {
        AudioEngine::Play("assets://sounds/pickup.wav");
        });

    eventBus.Subscribe<StartDragRequestEvent>([](const StartDragRequestEvent& e) {
        AudioEngine::Play("assets://sounds/put_on_conveyor.mp3");
        });

    eventBus.Subscribe<PlateGrabbedEvent>([](const PlateGrabbedEvent& e) {
        AudioEngine::Play("assets://sounds/pick_up.wav");
        });

    eventBus.Subscribe<IngredientUsedEvent>([](const IngredientUsedEvent& e) {
        AudioEngine::Play("assets://sounds/put_ingredient.mp3");
        });

    eventBus.Subscribe<CustomerServedEvent>([](const CustomerServedEvent& e) {
        });

    eventBus.Subscribe<ValidateOrderResponseEvent>([](const ValidateOrderResponseEvent& e) {
        if (e.IsCorrect) {
            AudioEngine::Play("assets://sounds/happy_customer.mp3");
        }
        else {
            AudioEngine::Play("assets://sounds/angry_customer.wav");
        }
        });

    eventBus.Subscribe<CarArrivedEvent>([](const CarArrivedEvent& e) {
        AudioEngine::Play("assets://sounds/truck_horn.mp3");
        });

    eventBus.Subscribe<PackageSpawnedEvent>([](const PackageSpawnedEvent& e) {
        AudioEngine::Play("assets://sounds/box_drop.mp3");
        });

    eventBus.Subscribe<LevelCompletedEvent>([this](const LevelCompletedEvent&) {
        m_IsLevelCompleted = true;
        });

    eventBus.Subscribe<OrderTakenEvent>([](const OrderTakenEvent& e) {
        AudioEngine::Play("assets://sounds/new_order.mp3");
    });

}

void GameLayer::UpdateTransformAnimations(World& world, float dt)
{
    auto* animatorStorage = world.GetComponentVector<TransformAnimatorComponent>();
    if (!animatorStorage) return;

    for (std::size_t i = 0; i < animatorStorage->dense.size(); ++i)
    {
        auto& animator = animatorStorage->dense[i];
        if (!animator.CurrentAnimation) continue;

        if (animator.IsPlaying)
        {
            float duration = animator.CurrentAnimation->GetDuration();
            animator.CurrentTime += animator.CurrentAnimation->GetTicksPerSecond() * dt * animator.PlaybackSpeed;

            if (animator.Loop) {
                animator.CurrentTime = std::fmod(animator.CurrentTime, duration);
            }
            else {
                if (animator.CurrentTime >= duration) {
                    animator.CurrentTime = duration;
                    animator.IsPlaying = false;
                }
            }
        }

        for (const auto& [trackName, targetEntityId] : animator.TargetEntities)
        {
            AnimationTrack* track = animator.CurrentAnimation->GetTrack(trackName);
            if (!track) continue;

            auto* transform = world.GetComponentByID<TransformComponent>(targetEntityId);
            if (transform)
            {
                transform->SetPosition(track->GetInterpolatedPosition(animator.CurrentTime));
                transform->SetRotation(track->GetInterpolatedRotationEuler(animator.CurrentTime));
                transform->SetScale(track->GetInterpolatedScale(animator.CurrentTime));
            }
        }
    }
}