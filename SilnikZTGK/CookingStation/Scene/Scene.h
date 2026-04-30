#pragma once
#include "Entity.h"
#include "../Core/Timestep.h"
#include "ecs.h"
#include <vector>


class ConveyorScript;
class Entity;
class Camera;

struct PlacementRequest {
	std::string Name = "";
	std::string Path = "";
	bool Active = false;
};

struct GridRequest {
	bool Active = false;
};

enum class SceneState {
	Edit = 0, Play = 1
};



class Scene
{
public:
	Scene();
	~Scene();

	SceneState GetState() const { return m_State; };
	void SetState(SceneState state) { m_State = state; };
	static std::shared_ptr<Scene> Copy(std::shared_ptr<Scene> other);

	void OnRuntimeStart(); // inicjalizacja silnika fizycznego i skryptow C#
	void OnRuntimeStop();  // czyszczenie pamiêci z  C# i niszczenie œwiata fizyki

	// Wywo³ywane z Twojego GameLayer::OnUpdate
	void OnUpdateRuntime(Timestep ts);

	// Wywo³ywane z EditorLayer::OnUpdate (jeœli macie edytor)
	//void OnUpdateEditor(Timestep ts, EditorCamera& camera);

	World& GetWorld() { return m_ECSWorld;  }

	std::vector<ConveyorScript*>& GetConveyors() { return Conveyors;  }

	void SetCamera(Camera* camera) { m_MainCamera = camera; }
	Camera* GetCamera() { return m_MainCamera; }

	void SetSelectedEntity(Entity e) { m_SelectedEntity = e; }
	Entity GetSelectedEntity() const { return m_SelectedEntity; }

	PlacementRequest& GetPlacementRequest() { return m_PlacementRequest; }
	void CalculateTransforms();
	void SetParent(Entity child, Entity parent);

	GridRequest& GetGridRequest() { return m_GridRequest; }
	void RebuildConveyorCache();
private:
	World m_ECSWorld;
	Camera* m_MainCamera = nullptr;
	Entity m_SelectedEntity = { std::numeric_limits<std::size_t>::max(), 0 };
	PlacementRequest m_PlacementRequest;
	GridRequest m_GridRequest;
	SceneState m_State = SceneState::Edit;

	std::vector<ConveyorScript*> Conveyors;
	bool m_ConveyorCacheReady = false;
};

