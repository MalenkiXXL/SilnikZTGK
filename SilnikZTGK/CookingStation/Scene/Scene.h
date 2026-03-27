#pragma once
#include "Entity.h"
#include "ecs.h"
#include <vector>

class Entity;
class Camera;

class Scene
{
public:
	Scene();
	~Scene();

	void Update();

	World& GetWorld() { return m_ECSWorld;  }

	void SetCamera(Camera* camera) { m_MainCamera = camera; }
	Camera* GetCamera() { return m_MainCamera; }

private:
	World m_ECSWorld;
	Camera* m_MainCamera = nullptr;
};

