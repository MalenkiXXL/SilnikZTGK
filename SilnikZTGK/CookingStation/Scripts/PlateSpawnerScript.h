#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include <string>

class PlateSpawnerScript : public ScriptableEntity
{
    float m_SpawnInterval = 3.0f;
    float m_TimeSinceLastSpawn = 0.0f;
    std::string m_PrefabPath = "CookingStation/Assets/prefabs/plate.json";
    int m_MaxPlates = 10;
    int m_ActivePlates = 0;

public:
    void OnCreate() override;
    void OnUpdate(Timestep ts) override;
    void onPlateDeSpawned() { m_ActivePlates--; }
private:
    void SpawnPrefab();
};