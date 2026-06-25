#include "SceneManager.h"

std::shared_ptr<Scene> SceneManager::s_ActiveScene = nullptr;

std::shared_ptr<Scene> SceneManager::GetActiveScene()
{
	return s_ActiveScene;
}

std::shared_ptr<Scene> SceneManager::NewScene()
{
	s_ActiveScene = std::make_shared<Scene>();
	return s_ActiveScene;
}

void SceneManager::SetActiveScene(std::shared_ptr<Scene> scene)
{
	s_ActiveScene = scene;
}

void SceneManager::Shutdown() {
	s_ActiveScene.reset();
}