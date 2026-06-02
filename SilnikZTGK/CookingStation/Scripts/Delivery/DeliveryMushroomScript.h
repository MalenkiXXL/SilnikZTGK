#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"

class DeliveryMushroomScript : public ScriptableEntity {
public:
    void OnCreate() override {
        auto* ac = GetComponent<AnimatorComponent>();
        if (ac && ac->AnimatorInstance) {
            ac->AnimatorInstance->PlayAnimation("Default");
            ac->IsPlaying = true;
        }
    }
    void OnUpdate(Timestep ts) override {}
};