#include "GameLayer.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include "CookingStation/Scene/ScriptableEntity.h"
#include <spdlog/spdlog.h> // Wymagane dla log�w diagnostycznych
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
    auto* animatorStorage = world.GetComponentVector<AnimatorComponent>();


    if (animatorStorage && !animatorStorage->dense.empty())
    {
        // spdlog::info("Animator dziala! Liczba postaci: {}", animatorStorage->dense.size());
    }
    else
    {
        // spdlog::debug("GameLayer: Brak komponent�w AnimatorComponent w aktywnej scenie.");
    }


    if (Input::IsMouseButtonJustPressed(0))
    {
        auto mousePos = Input::GetMousePosition();
        float mouseX = mousePos.first;
        float mouseY = mousePos.second;

        // Pobieramy rozmiar okna - domy�lnie pe�ny ekran
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
            // Aktualizujemy AspectRatio by proporcje klikniecia pokrywaly sie w 100% z kamera w grze
            float aspectRatio = viewWidth / (viewHeight > 0.0f ? viewHeight : 1.0f);
            camera->AspectRatio = aspectRatio;

            // Obliczamy rzutowanie promienia z kamery
            float orthoSize = 10.0f * (camera->Zoom / 45.0f);

            glm::mat4 proj3D = glm::ortho(-aspectRatio * orthoSize, aspectRatio * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
            glm::mat4 view3D = camera->GetViewMatrix();

            Ray ray = Physics::CastRayFromMouse(mouseX, mouseY, viewWidth, viewHeight, proj3D, view3D);

            // Wyszukujemy obiekt
            Entity closestEntity = Physics::GetHoveredEntity(ray, m_ActiveScene, true, true);

            // Je�li znaleziono jaki� obiekt, wywo�ujemy OnClick() na jego skryptach
            if (closestEntity.id != std::numeric_limits<std::size_t>::max())
            {
                auto* scriptStorage = world.GetComponentVector<NativeScriptComponent>();
                if (scriptStorage)
                {
                    auto* nsc = scriptStorage->Get(closestEntity);
                    if (nsc) {
                        for (auto& s : nsc->Scripts) {
                            if (s.Instance) {
                                s.Instance->OnClick();
                            }
                        }
                    }
                }
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