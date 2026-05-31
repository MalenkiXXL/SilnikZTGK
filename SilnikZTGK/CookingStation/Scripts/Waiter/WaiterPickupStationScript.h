#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Scripts/Plates/ItemScript.h"
#include "CookingStation/Events/GameEvents.h" 
#include <string>

class WaiterPickupStationScript : public ScriptableEntity
{
private:
    glm::ivec2 m_StationCell = { 0, 0 };

public:
    void OnCreate() override
    {
        auto* transform = GetComponent<TransformComponent>();
        if (transform) m_StationCell = GridSystem::WorldToCell(transform->GetPosition());
    }

    void OnUpdate(Timestep ts) override
    {
        auto* scene = GetScene();
        if (!scene) return;

        auto& world = scene->GetWorld();
        auto* transforms = world.GetComponentVector<TransformComponent>();
        auto* tags = world.GetComponentVector<TagComponent>();
        auto* scripts = world.GetComponentVector<NativeScriptComponent>();

        if (!transforms || !tags || !scripts) return;

        for (size_t i = 0; i < transforms->dense.size(); ++i)
        {
            Entity entity = transforms->reverse[i];
            if (entity.id == m_Entity.id) continue;

            if (GridSystem::WorldToCell(transforms->dense[i].GetPosition()) == m_StationCell)
            {
                auto* tagComp = tags->Get(entity);
                auto* nsc = scripts->Get(entity);

                if (tagComp && nsc)
                {
                    bool isItem = false;
                    for (auto& script : nsc->Scripts)
                    {
                        if (script.Name == "ItemScript") { isItem = true; break; }
                    }

                    if (isItem && tagComp->Tag != "PlateReady" && tagComp->Tag != "PlateCarried")
                    {
                        tagComp->Tag = "PlateReady";

                        // EVENT BUS: Powiadamiamy wszystkich kelnerów na sali, że talerz czeka
                        GetScene()->GetWorld().GetEventBus().Publish(PlateReadyEvent{ entity });
                    }
                }
            }
        }
    }
};