#include "GameLayer.h"
#include "CookingStation/Scene/SceneManager.h"

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
    // Dispatchowanie eventów (np. pauza gry, wyjœcie do menu)
}

