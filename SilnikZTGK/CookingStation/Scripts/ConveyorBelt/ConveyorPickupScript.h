#pragma once
#include "ConveyorScript.h"
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Events/GameEvents.h"

class ConveyorPickupScript : public ConveyorScript
{
private:
    std::size_t m_PlateGrabbedSubId = 0;

public:
    virtual void OnCreate() override
    {
        ConveyorScript::OnCreate();

        m_PlateGrabbedSubId = GetScene()->GetWorld().GetEventBus().Subscribe<PlateGrabbedEvent>(
                [this](const PlateGrabbedEvent& e) {
                    auto* scene = GetScene();
                    if (!scene) return;

                    auto* transforms = scene->GetWorld().GetComponentVector<TransformComponent>();
                    if (!transforms) return;

                    auto* plateTransform = transforms->Get(e.Plate);
                    auto* myTransform = GetComponent<TransformComponent>();

                    if (plateTransform && myTransform)
                    {
                        glm::ivec2 plateCell = GridSystem::WorldToCell(plateTransform->GetPosition());
                        glm::ivec2 myCell = GridSystem::WorldToCell(myTransform->GetPosition());

                        if (plateCell == myCell)
                        {
                            this->IsOccupied = false;
                        }
                    }
                }
        );
    }

    virtual void OnDestroy() override
    {
        GetScene()->GetWorld().GetEventBus().Unsubscribe<PlateGrabbedEvent>(m_PlateGrabbedSubId);
        ConveyorScript::OnDestroy();
    }
};