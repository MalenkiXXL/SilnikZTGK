#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include <glm/glm.hpp>
#include <string>

enum class DeliveryState {
    DRIVING_IN,
    ANIMATING_OPEN,
    DROPPING,
    ANIMATING_CLOSE,
    DRIVING_OUT
};

class DeliveryCarScript : public ScriptableEntity
{
public:
    static inline const glm::vec3 m_StartPos = { -17.0f, 5.0f, 100.0f };

    DeliveryState m_State = DeliveryState::DRIVING_IN;
    glm::vec3 m_DropPos  = { -17.0f, 5.0f, 5.0f };
    glm::vec3 m_ExitPos  = { -17.0f, 5.0f, -120.0f };

    float m_Speed = 8.0f;
    float m_AnimationTimer = 0.0f;

    DeliveryCarScript() = default;

    void OnCreate() override;
    void OnUpdate(Timestep ts) override;
    void OnDestroy() override;

private:
    std::size_t m_CollectedSubId = 0;
    bool m_ArePackagesCollected = false;

    bool m_MushroomSpawned = false;
    Entity m_MushroomEntity = { std::numeric_limits<std::size_t>::max(), 0 };
};