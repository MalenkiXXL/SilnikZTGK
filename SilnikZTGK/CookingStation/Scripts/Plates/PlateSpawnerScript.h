#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include <string>
#include <vector>

class PlateSpawnerScript : public ScriptableEntity
{
    float m_SpawnInterval = 3.0f;
    float m_TimeSinceLastSpawn = 0.0f;
    std::string m_PrefabPath = "assets://prefabs/plate.json";

    int m_ActivePlates = 0;
    std::vector<Entity> m_VisualStack;
    bool m_PendingGoldFlash = false;

public:
    void OnCreate() override;
    void OnDestroy() override;
    void OnUpdate(Timestep ts) override;
    void onPlateDeSpawned() { m_ActivePlates--; }
private:
    void SpawnPrefab();
    int CalculateMaxPlates();
    void UpdateVisualStack(int targetCount);
};