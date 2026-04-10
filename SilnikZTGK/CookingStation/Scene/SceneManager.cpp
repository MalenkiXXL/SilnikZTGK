#include "SceneManager.h"

// Inicjalizacja statycznego wskaŸnika
std::shared_ptr<Scene> SceneManager::s_ActiveScene = nullptr;

std::shared_ptr<Scene> SceneManager::GetActiveScene()
{
	return s_ActiveScene;
}

std::shared_ptr<Scene> SceneManager::NewScene()
{
	// Tworzymy now¹ scenê. Poprzednia (jeœli nikt inny jej nie trzyma) 
	// zostanie automatycznie usuniêta z pamiêci dziêki std::shared_ptr.
	s_ActiveScene = std::make_shared<Scene>();
	return s_ActiveScene;
}

void SceneManager::SetActiveScene(std::shared_ptr<Scene> scene)
{
	s_ActiveScene = scene;
}