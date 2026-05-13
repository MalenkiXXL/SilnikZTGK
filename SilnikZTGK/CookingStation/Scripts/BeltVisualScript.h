#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"

class BeltVisualScript : public ScriptableEntity
{
public:
    float Speed = 2.0f;

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
    }

    void OnUpdate(Timestep ts) override
    {
        auto* scroll = GetComponent<UVScrollComponent>();
        if (scroll)
        {
            float modelLength = 4.0f;
            float uvSpeed = Speed / modelLength;

            scroll->Offset += uvSpeed * ts;

            if (scroll->Offset > 1.0f) scroll->Offset -= 1.0f;
            if (scroll->Offset < 0.0f) scroll->Offset += 1.0f;
        }
    }
};