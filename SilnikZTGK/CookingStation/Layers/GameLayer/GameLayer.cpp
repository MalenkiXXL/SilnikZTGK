#include "GameLayer.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Core/AudioEngine.h"

void GameLayer::OnAttach()
{
    // Zamiast tworzyæ logikê tutaj, ³adujemy scenê, któr¹ wyplu³ edytor/serializer.
    // Jeœli to tryb "Play" w edytorze, tutaj nastêpuje kopia sceny edytorowej do m_ActiveScene.
    // Jeœli to standalone, ³adujemy z pliku.
    m_ActiveScene = std::make_shared<Scene>();

    // Wywo³ujemy Start na scenie, OnCreate()
    m_ActiveScene->OnRuntimeStart();
}

void GameLayer::OnDetach()
{
    m_ActiveScene->OnRuntimeStop();
}

void GameLayer::OnUpdate(Timestep ts)
{
    // GameLayer zajmuje siê logik¹ sceny

    // To tutaj odpalane s¹ systemy ECS (Fizyka, Skrypty, Animacje).
    m_ActiveScene->OnUpdateRuntime(ts);
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
    if (e.GetKeyCode() == 32 && e.GetRepeatCode() == 0)
    {
        AudioEngine::Play("CookingStation/Assets/sounds/onion_chopping.mp3");
        return false;
    }

    return false;
}