#pragma once
#include "Scene.h"
#include <memory>
#include <string>

class SceneManager
{
public:
	// Zwraca aktualnie aktywn¹ scenê
	static std::shared_ptr<Scene> GetActiveScene();

	// Tworzy czyst¹, now¹ scenê i ustawia j¹ jako aktywn¹
	static std::shared_ptr<Scene> NewScene();

	// Zastêpuje obecn¹ scenê inn¹ 
	static void SetActiveScene(std::shared_ptr<Scene> scene);

private:
	static std::shared_ptr<Scene> s_ActiveScene;
};