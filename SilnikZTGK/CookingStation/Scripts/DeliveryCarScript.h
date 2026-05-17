#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include <glm/glm.hpp>
#include <string>

enum class DeliveryState {
    IDLE,
    DRIVING_IN,
    DROPPING,
    DRIVING_OUT
};

class DeliveryCarScript : public ScriptableEntity
{
public:
    inline static DeliveryCarScript* s_Instance = nullptr;
    DeliveryState m_State = DeliveryState::IDLE;

    glm::vec3 m_StartPos = { -11.0f, 2.0f, 30.0f };
    glm::vec3 m_DropPos  = { -11.0f, 2.0f, 5.0f };
    glm::vec3 m_ExitPos  = { -11.0f, 2.0f, -25.0f };

    float m_Speed = 8.0f;
    float m_WaitTimer = 0.0f;

    std::string m_PackagePrefabPath = "CookingStation/Assets/prefabs/package.json";

    bool NeedsDelivery = false;

    void OnCreate() override;
    void OnUpdate(Timestep ts) override;

private:
    void SpawnPackages();
};