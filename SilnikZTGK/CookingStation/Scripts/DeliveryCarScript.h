#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include <glm/glm.hpp>
#include <string>

enum class DeliveryState {
    DRIVING_IN,
    DROPPING,
    DRIVING_OUT
};

class DeliveryCarScript : public ScriptableEntity
{
public:
    static inline const glm::vec3 m_StartPos = { -11.0f, 2.0f, 30.0f };

    DeliveryState m_State = DeliveryState::DRIVING_IN;
    glm::vec3 m_DropPos  = { -11.0f, 2.0f, 5.0f };
    glm::vec3 m_ExitPos  = { -11.0f, 2.0f, -25.0f };

    float m_Speed = 8.0f;
    float m_WaitTimer = 0.0f;

    std::string m_PackagePrefabPath = "CookingStation/Assets/prefabs/package.json";

    void OnCreate() override;
    void OnUpdate(Timestep ts) override;

private:
    void SpawnPackages();
};