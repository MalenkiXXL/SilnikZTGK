#include "CookingStation/Scripts/Plates/PlateSpawnerScript.h"
#include "CookingStation/Scene/PrefabSerializer.h"

void PlateSpawnerScript::OnCreate()
{
    m_TimeSinceLastSpawn = m_SpawnInterval;
}

void PlateSpawnerScript::OnUpdate(Timestep ts)
{
    m_TimeSinceLastSpawn += ts.GetSeconds();

    if (m_TimeSinceLastSpawn >= m_SpawnInterval)
    {
        int currentPlates = 0;
        auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        if (tags)
        {
            for (const auto& tagComp : tags->dense)
            {
                if (tagComp.Tag.find("Plate") != std::string::npos || tagComp.Tag.find("Talerz") != std::string::npos)
                {
                    currentPlates++;
                }
            }
        }

        if (currentPlates < m_MaxPlates)
        {
            SpawnPrefab();
        }

        m_TimeSinceLastSpawn = 0.0f;
    }
}

void PlateSpawnerScript::SpawnPrefab()
{
    auto* transform = GetComponent<TransformComponent>();
    if (!transform) return;

    glm::vec3 spawnPos = transform->GetPosition();

    PrefabSerializer::Deserialize(GetScene(), m_PrefabPath, spawnPos);
}