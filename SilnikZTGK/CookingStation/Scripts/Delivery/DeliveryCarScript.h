#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/AudioEngine.h"
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
    static inline const glm::vec3 m_StartPos = { -18.0f, 5.0f, 100.0f };

    DeliveryState m_State = DeliveryState::DRIVING_IN;
    glm::vec3 m_DropPos  = { -18.0f, 5.0f, 5.0f };
    glm::vec3 m_ExitPos  = { -18.0f, 5.0f, -120.0f };

    float m_Speed = 8.0f;
    float m_AnimationTimer = 0.0f;

    DeliveryCarScript() = default;

    void OnCreate() override;
    void OnUpdate(Timestep ts) override;
    void OnDestroy() override;

private:
    std::size_t m_CollectedSubId = 0;
    bool m_ArePackagesCollected = false;

    std::size_t m_MushroomSubId = 0;
    bool m_MushroomSpawned = false;
    std::size_t m_MushroomEntity = NULL_ENTITY;

    ma_sound* m_EngineSound = nullptr;
    bool m_EngineSoundStarted = false;

    std::string m_TireTrackPrefabPath = "CookingStation/Assets/prefabs/tire_track.json";
    void SpawnTireTracks();
    bool m_TireTracksSpawned = false;

    std::size_t m_PauseSubId = 0;
    std::size_t m_ResumeSubId = 0;

};