#include "GameLayer.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Core/AudioEngine.h"
#include <spdlog/spdlog.h> // Wymagane dla logów diagnostycznych

void GameLayer::OnAttach()
{
    // PRZED: m_ActiveScene = std::make_shared<Scene>();
    // PO: pobieramy tê sam¹ scenê któr¹ wype³ni³ AssetLayer
    m_ActiveScene = SceneManager::GetActiveScene();

    if (!m_ActiveScene)
    {
        spdlog::error("GameLayer: Brak aktywnej sceny w SceneManager!");
        return;
    }

    m_ActiveScene->OnRuntimeStart();
}

void GameLayer::OnDetach()
{
    if (m_ActiveScene)
        m_ActiveScene->OnRuntimeStop();
}

void GameLayer::OnUpdate(Timestep ts)
{
    if (!m_ActiveScene) return;

    // GameLayer zajmuje siê logik¹ sceny
    // To tutaj odpalane s¹ systemy ECS (w tym nasz nowy Animator przeniesiony do Scene.cpp)
    m_ActiveScene->OnUpdateRuntime(ts);

    // ====================================================================
    // DIAGNOSTYKA: Sprawdzamy czy komponent przetrwa³ wejœcie w tryb Play
    // ====================================================================
    if (m_ActiveScene->GetState() == SceneState::Play)
    {
        auto& world = m_ActiveScene->GetWorld();
        auto* animatorStorage = world.GetComponentVector<AnimatorComponent>();

        if (animatorStorage && !animatorStorage->dense.empty())
        {
            // Opcjonalnie: odkomentuj poni¿sz¹ liniê na chwilê, by sprawdziæ czy wykrywa grzybka
            // spdlog::info("Animator dziala! Liczba postaci: {}", animatorStorage->dense.size());
        }
        else
        {
            // Jeœli to siê wywo³a - oznacza to, ¿e serializer niszczy nasz komponent!
            spdlog::error("BRAK ANIMATORA! Komponent wyparowal w trybie Play!");
        }
    }
}

void GameLayer::OnEvent(Event& e)
{
    // Tworzymy dyspozytor z rzuconym zdarzeniem
    EventDispatcher dispatcher(e);

    // Przekazujemy zdarzenia typu KeyPressedEvent do naszej metody OnKeyPressed.
    // U¿ywamy tu lambdy, ¿eby elegancko przekazaæ wskaŸnik 'this'.
    dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& event) {
        return OnKeyPressed(event);
        });
}

bool GameLayer::OnKeyPressed(KeyPressedEvent& e)
{
    // 1. Pobieramy aktualn¹ scenê z mened¿era
    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();

    // 2. Jeœli nie ma sceny, ALBO nie jest ona w trybie PLAY - ignorujemy klikniêcia gracza!
    if (!activeScene || activeScene->GetState() != SceneState::Play)
    {
        return false;
    }

    // 3. W³aœciwa logika GRY (wykona siê tylko w trybie PLAY)
    if (e.GetKeyCode() == 32 && e.GetRepeatCode() == 0) // Spacja
    {
        AudioEngine::Play("CookingStation/Assets/sounds/onion_chopping.mp3");
        return false;
    }

    return false;
}