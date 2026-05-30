#pragma once
#include "Entity.h"
#include "../Core/Timestep.h"
#include "ecs.h"
#include <vector>
#include <unordered_map>
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Events/GameEvents.h"

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
	Edit = 0, Play = 1, Pause = 2
};


struct GridPos {
	int x, z;
	bool operator==(const GridPos& o) const { return x == o.x && z == o.z; }
};

struct GridPosHash {
	size_t operator()(const GridPos& p) const {
		return std::hash<int>()(p.x) ^ (std::hash<int>()(p.z) << 16);
	}
};

class Scene
{
public:
	Scene();
	~Scene();

	SceneState GetState() const { return m_State; };
	void SetState(SceneState state) { m_State = state; };

    void SetViewportSize(uint32_t width, uint32_t height) {
        m_ViewportWidth = width;
        m_ViewportHeight = height;
    }

    uint32_t GetViewportWidth() const { return m_ViewportWidth; }
    uint32_t GetViewportHeight() const { return m_ViewportHeight; }

	static std::shared_ptr<Scene> Copy(std::shared_ptr<Scene> other);

	void OnRuntimeStart(); // inicjalizacja silnika fizycznego i skryptow C#
	void OnRuntimeStop();  // czyszczenie pami�ci z  C# i niszczenie �wiata fizyki

	void OnUpdateRuntime(Timestep ts);


	World& GetWorld() { return m_ECSWorld;  }

	ConveyorScript* GetConveyorAt(float worldX, float worldZ);
	std::unordered_map<GridPos, ConveyorScript*, GridPosHash>& GetConveyorMap() { return ConveyorMap; }

	void SetCamera(Camera* camera) { m_MainCamera = camera; }
	Camera* GetCamera() { return m_MainCamera; }

	void SetSelectedEntity(Entity e) { m_SelectedEntity = e; }
	Entity GetSelectedEntity() const { return m_SelectedEntity; }

	PlacementRequest& GetPlacementRequest() { return m_PlacementRequest; }
	void CalculateTransforms();

    Entity GetParent(Entity child);
    void SetParent(Entity child, Entity parent);
    void RemoveParent(Entity child);

	GridRequest& GetGridRequest() { return m_GridRequest; }
	void RebuildConveyorCache();

	//Nowy interfejsc SSA
	void UpdateSpatialGrid();
	const std::vector<Entity>* GetEntitiesInCell(const glm::ivec2& cell) const;

    const std::vector<glm::vec3>& GetPickupPoints() const { return m_PickupPoints; }
    void AddPickupPoint(const glm::vec3& position) { m_PickupPoints.push_back(position); }
    void DestroyEntity(Entity entity);

private:
	World m_ECSWorld;
	Camera* m_MainCamera = nullptr;
	Entity m_SelectedEntity = { std::numeric_limits<std::size_t>::max(), 0 };
	PlacementRequest m_PlacementRequest;
	GridRequest m_GridRequest;
	SceneState m_State = SceneState::Edit;

	std::unordered_map<GridPos, ConveyorScript*, GridPosHash> ConveyorMap;
    std::vector<glm::vec3> m_PickupPoints;
	bool m_ConveyorCacheReady = false;

    uint32_t m_ViewportWidth = 0;
    uint32_t m_ViewportHeight = 0;

    std::vector<Entity> m_EntitiesToDestroy;

	//struktura SSA: komorka siatki -> lista encji wewnatrz niej
	std::unordered_map<glm::ivec2, std::vector<Entity>, IVec2Hash> m_SpartialGrid;
	std::size_t m_DestroySubId;
	void OnEntityDestroyRequest(const EntityDestroyRequestEvent& e);
};

