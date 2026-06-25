#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Events/GameEvents.h" 
#include <glm/glm.hpp>
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/AudioEngine.h"

struct AngleDirection {
    float angle;
    glm::vec3 direction;
};

static constexpr AngleDirection s_Mappings[] = {
    {  90.0f, { 0.0f, 0.0f,  1.0f } },
    { 270.0f, { 0.0f, 0.0f, -1.0f } },
    { 180.0f, { 1.0f, 0.0f,  0.0f } },
    {   0.0f, {-1.0f, 0.0f,  0.0f } },
};

class ConveyorScript : public ScriptableEntity
{
public:
    glm::vec3 PushDirection = { 0.0f, 0.0f, 0.0f };
    float Speed = 2.0f;
    bool IsOccupied = false;
    bool IsJammed = false;

    void OnCreate() override
    {
        SetPushDirection();
    }

    void OnDestroy() override
    {

    }

    virtual void HandleClick()
    {
    }

    void SetPushDirection()
    {
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        float rotY = transform->GetRotation().y;

        float normalizedRot = fmodf(rotY, 360.0f);
        if (normalizedRot < 0.0f) normalizedRot += 360.0f;

        for (auto& m : s_Mappings)
        {
            if (std::abs(normalizedRot - m.angle) < 5.0f)
            {
                PushDirection = m.direction;
                return;
            }
        }

        PushDirection = s_Mappings[3].direction;
    }

};
