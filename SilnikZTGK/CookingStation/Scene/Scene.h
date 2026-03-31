#pragma once
#include "Entity.h"
#include "ecs.h"
#include <vector>

class Entity;
class Camera;

struct PlacementRequest {
	std::string Name = "";
	std::string Path = "";
	bool Active = false;
};

class Scene
{
public:
	Scene();
	~Scene();

	void Update();

	World& GetWorld() { return m_ECSWorld;  }

	void SetCamera(Camera* camera) { m_MainCamera = camera; }
	Camera* GetCamera() { return m_MainCamera; }

	void SetSelectedEntity(Entity e) { m_SelectedEntity = e; }
	Entity GetSelectedEntity() const { return m_SelectedEntity; }

	PlacementRequest& GetPlacementRequest() { return m_PlacementRequest; }

private:
	World m_ECSWorld;
	Camera* m_MainCamera = nullptr;
	Entity m_SelectedEntity = { std::numeric_limits<std::size_t>::max(), 0 };
	PlacementRequest m_PlacementRequest;
};

