#include "GameLayer.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Events/GameEvents.h" // DODANE: Wymagane dla EntityClickedEvent
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

    if (m_ActiveScene->GetState() != SceneState::Play)
    {
        return;
    }

    m_ActiveScene->OnUpdateRuntime(ts);

    auto& world = m_ActiveScene->GetWorld();

    if (Input::IsMouseButtonJustPressed(0))
    {
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

            Ray ray = Physics::CastRayFromMouse(mouseX, mouseY, viewWidth, viewHeight, proj3D, view3D);

            Entity closestEntity = Physics::GetHoveredEntity(ray, m_ActiveScene, true, true);

            // ZMIANA: Zamiast przeszukiwać NativeScriptComponent, publikujemy zdarzenie!
            if (closestEntity.id != std::numeric_limits<std::size_t>::max())
            {
                // To wywoła HandleClick() we wszystkich skryptach, które zasubskrybowały to zdarzenie.
                world.GetEventBus().Publish(EntityClickedEvent{ closestEntity, 0 });
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