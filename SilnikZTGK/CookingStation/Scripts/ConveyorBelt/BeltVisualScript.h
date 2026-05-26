#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scripts/ConveyorBelt/ConveyorScript.h"
#include "CookingStation/Events/GameEvents.h" // Dodaj to!

class BeltVisualScript : public ScriptableEntity
{
private:
    float Speed = 2.0f;
    ConveyorScript* m_ParentConveyor = nullptr;
    std::size_t m_ClickSubId = 0; // Dodaj pole na subskrypcjê

public:
    void OnCreate() override
    {
        auto* existingScroll = GetComponent<UVScrollComponent>();
        if (!existingScroll)
        {
            UVScrollComponent scroll;
            scroll.Speed = Speed;
            scroll.Offset = 0.0f;
            AddComponent<UVScrollComponent>(scroll);
        }

        m_ParentConveyor = GetParentScript<ConveyorScript>();

        // 1. Subskrybujemy klikniêcia
        m_ClickSubId = GetScene()->GetWorld().GetEventBus().Subscribe<EntityClickedEvent>(
            [this](const EntityClickedEvent& e) {
                // Jeœli klikniêto w ten wizualny obiekt taœmy, przeka¿ klikniêcie do rodzica (ConveyorScript)
                if (e.TargetEntity.id == this->m_Entity.id)
                {
                    this->HandleClick();
                }
            }
        );
    }

    void OnDestroy() override
    {
        GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityClickedEvent>(m_ClickSubId);
    }

    void OnUpdate(Timestep ts) override
    {
        auto* scroll = GetComponent<UVScrollComponent>();
        if (!scroll) return;

        auto* conveyor = GetParentScript<ConveyorScript>();
        if (conveyor && conveyor->IsJammed)
            return;

        float modelLength = 4.0f;
        float uvSpeed = Speed / modelLength;

        scroll->Offset += uvSpeed * ts;

        if (scroll->Offset > 1.0f) scroll->Offset -= 1.0f;
        if (scroll->Offset < 0.0f) scroll->Offset += 1.0f;
    }

    // Nowa metoda obs³uguj¹ca klikniêcie (zastêpuje OnClick)
    void HandleClick()
    {
        if (m_ParentConveyor)
        {
            // Przekazujemy logikê do ConveyorScript
            m_ParentConveyor->HandleClick();
        }
    }
};