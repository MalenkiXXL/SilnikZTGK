#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scripts/ConveyorScript.h"

class BeltVisualScript : public ScriptableEntity
{
private:
    float Speed = 2.0f;
    ConveyorScript* m_ParentConveyor = nullptr;

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

    void OnClick() override
    {
        if (m_ParentConveyor)
        {
            m_ParentConveyor->OnClick();
        }
    }
};