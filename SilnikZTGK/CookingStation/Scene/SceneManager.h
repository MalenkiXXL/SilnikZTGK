#pragma once
#include "Scene.h"
#include <memory>
#include <string>

class SceneManager
{
public:
	static std::shared_ptr<Scene> GetActiveScene();

	static std::shared_ptr<Scene> NewScene();

	static void SetActiveScene(std::shared_ptr<Scene> scene);

private:
	static std::shared_ptr<Scene> s_ActiveScene;
};