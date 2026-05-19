#include "GameLayer.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Core/AudioEngine.h"
#include <spdlog/spdlog.h> // Wymagane dla logów diagnostycznych

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
        spdlog::debug("GameLayer: Brak komponentów AnimatorComponent w aktywnej scenie.");
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
