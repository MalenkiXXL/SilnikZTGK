#include "CookingStation/Scripts/Plates/PlateSpawnerScript.h"
#include "CookingStation/Scene/PrefabSerializer.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h" 
#include "CookingStation/Layers/AssetLayer/AssetManager.h"   
#include "CookingStation/Events/GameEvents.h"

void PlateSpawnerScript::OnCreate()
{
    m_TimeSinceLastSpawn = 0.0f;
}

void PlateSpawnerScript::OnDestroy()
{
    for (auto entity : m_VisualStack) {
        GetScene()->DestroyEntity(entity);
    }
    m_VisualStack.clear();
}

int PlateSpawnerScript::CalculateMaxPlates()
{
    if (GameManagerScript::s_IsTutorialMode) {
        return 1;
    }

    // --- Oryginalna logika dla normalnych map ---
    int maxPlates = 6;

    if (GameManagerScript::s_Instance) {
        if (GameManagerScript::s_Instance->GetQuestState() == QuestEventState::QuestActive) {
            auto* quest = GameManagerScript::s_Instance->GetCurrentQuest();
            if (quest) {
                int portionsLeft = quest->Portions - GameManagerScript::s_Instance->GetQuestProgress();
                if (portionsLeft > 0) {
                    maxPlates += portionsLeft;
                }
            }
        }
    }

    return maxPlates;
}

void PlateSpawnerScript::UpdateVisualStack(int targetCount)
{
    glm::vec3 basePos = glm::vec3(0.0f);
    bool foundTarget = false;

    auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
    if (tags) {
        for (size_t i = 0; i < tags->dense.size(); ++i) {
            if (tags->dense[i].Tag == "TasmaStosTalerzy") {
                Entity targetEntity = tags->reverse[i];
                auto* targetTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(targetEntity);

                if (targetTransform) {
                    basePos = targetTransform->GetPosition() + glm::vec3(0.0f, 1.2f, 0.0f);
                    foundTarget = true;
                    break;
                }
            }
        }
    }

    if (!foundTarget) {
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;
        basePos = transform->GetPosition() + glm::vec3(0.6f, 1.2f, 0.0f);
    }

    while (m_VisualStack.size() > targetCount) {
        GetScene()->DestroyEntity(m_VisualStack.back());
        m_VisualStack.pop_back();
    }

    bool stackGrew = false;

    while (m_VisualStack.size() < targetCount) {
        auto builder = GetScene()->GetWorld().BuildEntity();

        TransformComponent tc;
        tc.SetPosition(basePos + glm::vec3(0.0f, m_VisualStack.size() * 0.15f, 0.0f));
        tc.SetScale(glm::vec3(1.0f));
        builder.With<TransformComponent>(tc);

        MeshComponent mesh;
        mesh.ModelPtr = AssetManager::GetModel("assets://models/przybory_kuchenne/naczynia/plate-white.gltf");
        builder.With<MeshComponent>(mesh);

        builder.With<TagComponent>({ "VisualStackPlate" });

        m_VisualStack.push_back(builder.Build());

        stackGrew = true;
    }

    if (stackGrew) {
        m_PendingGoldFlash = true;
    }
}

void PlateSpawnerScript::OnUpdate(Timestep ts)
{

    auto* transform = GetComponent<TransformComponent>();
    if (transform && transform->GetPosition().y < -50.0f) {
        while (!m_VisualStack.empty()) {
            GetScene()->DestroyEntity(m_VisualStack.back());
            m_VisualStack.pop_back();
        }
        return;
    }

    if (m_PendingGoldFlash) {
        for (auto entity : m_VisualStack) {
            TriggerHighlightEvent ev;
            ev.TargetEntity = entity;
            ev.Color = glm::vec3(1.0f, 0.8f, 0.0f);
            ev.Duration = 1.2f;                     
            ev.IsInfinite = false;

            GetScene()->GetWorld().GetEventBus().Publish(ev);
        }
        m_PendingGoldFlash = false;
    }

    int currentPlates = 0;
    auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();

    if (tags)
    {
        for (const auto& tagComp : tags->dense)
        {
            if (tagComp.Tag.find("VisualStack") != std::string::npos ||
                tagComp.Tag.find("Tasma") != std::string::npos ||
                tagComp.Tag.find("Spawner") != std::string::npos)
            {
                continue;
            }

            if (tagComp.Tag.find("Plate") != std::string::npos || tagComp.Tag.find("Talerz") != std::string::npos)
            {
                currentPlates++;
            }
        }
    }

    int currentMaxPlates = CalculateMaxPlates();
    int platesWaitingToSpawn = std::max(0, currentMaxPlates - currentPlates);

    UpdateVisualStack(platesWaitingToSpawn);

    m_TimeSinceLastSpawn += ts.GetSeconds();

    if (m_TimeSinceLastSpawn >= m_SpawnInterval)
    {
        if (currentPlates < currentMaxPlates)
        {
            SpawnPrefab();
            m_TimeSinceLastSpawn = 0.0f;

            UpdateVisualStack(std::max(0, currentMaxPlates - (currentPlates + 1)));

            for (auto entity : m_VisualStack) {
                TriggerHighlightEvent ev;
                ev.TargetEntity = entity;
                ev.Color = glm::vec3(0.3f, 1.0f, 0.3f);
                ev.Duration = 0.8f;
                ev.IsInfinite = false;

                GetScene()->GetWorld().GetEventBus().Publish(ev);
            }
        }
    }
}

void PlateSpawnerScript::SpawnPrefab()
{
    auto* transform = GetComponent<TransformComponent>();
    if (!transform) return;

    glm::vec3 spawnPos = transform->GetPosition();
    PrefabSerializer::Deserialize(GetScene(), m_PrefabPath, spawnPos);
}