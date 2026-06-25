#include "CookingStation/Scripts/Delivery/DeliveryCarScript.h"
#include <spdlog/spdlog.h>
#include "CookingStation/miniaudio.h"
#include "CookingStation/Scene/PrefabSerializer.h"
#include "CookingStation/Core/Application.h"
#include "CookingStation/Events/GameEvents.h"

void DeliveryCarScript::OnCreate()
{
    m_CollectedSubId = GetScene()->GetWorld().GetEventBus().Subscribe<DeliveryCollectedEvent>(
            [this](const DeliveryCollectedEvent& e) {
                this->m_ArePackagesCollected = true;
            }
    );

    m_MushroomSubId = GetScene()->GetWorld().GetEventBus().Subscribe<DeliveryMushroomAppearedEvent>(
            [this](const DeliveryMushroomAppearedEvent& e) {
            }
    );

    auto* animator = GetComponent<TransformAnimatorComponent>();
    if (!animator) return;

    std::unordered_set<std::string> animatedNodeNames;
    for (const auto& [clipName, animPtr] : animator->Animations) {
        if (animPtr) {
            for (const auto& [trackName, track] : animPtr->GetTracks())
                animatedNodeNames.insert(trackName);
        }
    }

    auto& world = GetScene()->GetWorld();

    auto* myRel = world.GetComponent<RelationshipComponent>(m_Entity);
    if (!myRel) return;

    std::size_t childId = myRel->FirstChild;
    while (childId != NULL_ENTITY)
    {
        auto* childRel = world.GetComponentByID<RelationshipComponent>(childId);
        auto* childTag = world.GetComponentByID<TagComponent>(childId);

        if (childTag)
        {
            for (const std::string& animNodeName : animatedNodeNames)
            {
                if (childTag->Tag.find(animNodeName) == 0)
                {
                    animator->TargetEntities[animNodeName] = childId;
                    spdlog::info("[DeliveryCar] Podpieto klapę '{}' do animacji", childTag->Tag);
                }
            }

            if (childTag->Tag.find("DeliveryMushroom") == 0)
            {
                m_MushroomEntity = childId; // <--- ZMIANA
                m_MushroomSpawned = true;
                spdlog::info("[DeliveryCar] Znaleziono grzyba: '{}'", childTag->Tag);
            }        }

        childId = childRel ? childRel->NextSibling : NULL_ENTITY;
    }

    m_EngineSound = AudioEngine::PlayLoopingSound("assets://sounds/car_engine.mp3", 0.3f, true);
    if (m_EngineSound) {
        ma_sound_set_volume(m_EngineSound, 0.0f);
        spdlog::info("[DeliveryCar] Załadowano dźwięk silnika");
    } else {
        spdlog::warn("[DeliveryCar] Nie udało się załadować dźwięku silnika");
    }

    auto& appBus = Application::Get().GetEventBus();

    m_PauseSubId = appBus.Subscribe<GamePausedEvent>(
            [this](const GamePausedEvent&) {
                if (m_EngineSound && m_EngineSoundStarted) {
                    ma_sound_stop(m_EngineSound);
                }
            }
    );

    m_ResumeSubId = appBus.Subscribe<GameResumedEvent>(
            [this](const GameResumedEvent&) {
                if (m_EngineSound && m_EngineSoundStarted && AudioEngine::AreSoundsEnabled()) {
                    AudioEngine::ReplaySound(m_EngineSound);
                }
            }
    );

    m_MainMenuSubId = appBus.Subscribe<ShowMainMenuEvent>(
            [this](const ShowMainMenuEvent&) {
                if (m_EngineSound) {
                    ma_sound_stop(m_EngineSound);
                    m_EngineSoundStarted = false;
                }
            }
    );
}

void DeliveryCarScript::OnDestroy()
{
    GetScene()->GetWorld().GetEventBus().Unsubscribe<DeliveryCollectedEvent>(m_CollectedSubId);
    GetScene()->GetWorld().GetEventBus().Unsubscribe<DeliveryMushroomAppearedEvent>(m_MushroomSubId);

    auto& appBus = Application::Get().GetEventBus();
    if (m_PauseSubId != 0) appBus.Unsubscribe<GamePausedEvent>(m_PauseSubId);
    if (m_ResumeSubId != 0) appBus.Unsubscribe<GameResumedEvent>(m_ResumeSubId);
    if (m_MainMenuSubId != 0) appBus.Unsubscribe<ShowMainMenuEvent>(m_MainMenuSubId);

    if (m_EngineSound) {
        AudioEngine::StopLoopingSound(m_EngineSound);
        m_EngineSound = nullptr;
    }
}

void DeliveryCarScript::OnUpdate(Timestep ts)
{
    auto* transform = GetComponent<TransformComponent>();
    auto* animator = GetComponent<TransformAnimatorComponent>();
    if (!transform) return;

    glm::vec3 currentPos = transform->GetPosition();

    float hiddenY = -0.5f;
    float poppedY = 1.0f;
    float zOffset = 0.0f;
    float xOffset = 2.5f;

    float engineVolume = 0.0f;
    bool shouldPlayEngine = false;

    switch (m_State)
    {
        case DeliveryState::DRIVING_IN:
        {
            shouldPlayEngine = true;
            engineVolume = 0.2f;

            glm::vec3 dir = m_DropPos - currentPos;
            float dist = glm::length(dir);

            float stepDistance = m_Speed * (float)ts.GetSeconds();

            if (dist < stepDistance)
            {
                transform->SetPosition(m_DropPos);

                if (animator) {
                    animator->IsPlaying = true;
                    animator->PlaybackSpeed = 0.417f; //bo 15 klatek w blenderze
                    animator->PlayAnimation("Open", true, false);
                }

                m_State = DeliveryState::ANIMATING_OPEN;
                m_AnimationTimer = 1.5f;

                spdlog::info("[DeliveryCar] Dojechalem. Otwieram klape!");
            }
            else
            {
                dir = glm::normalize(dir);
                currentPos += dir * m_Speed * (float)ts.GetSeconds();
                transform->SetPosition(currentPos);
            }
            break;
        }

        case DeliveryState::ANIMATING_OPEN:
        {
            shouldPlayEngine = true;
            float progress = 1.0f - (m_AnimationTimer / 1.5f);
            progress = glm::clamp(progress, 0.0f, 1.0f);
            engineVolume = glm::mix(0.2f, 0.1f, progress);

            m_AnimationTimer -= (float)ts;

            if (m_MushroomSpawned) {
                auto* mushTransform = GetScene()->GetWorld().GetComponentByID<TransformComponent>(m_MushroomEntity);
                if (mushTransform) {
                    float progress = 1.0f - (m_AnimationTimer / 1.5f);
                    progress = glm::clamp(progress, 0.0f, 1.0f);

                    float currentY = glm::mix(hiddenY, poppedY, progress);
                    mushTransform->SetPosition({xOffset, currentY, zOffset});
                }
            }

            if (m_AnimationTimer <= 0.0f)
            {
                m_State = DeliveryState::DROPPING;
                if (animator) animator->IsPlaying = false;

                GetScene()->GetWorld().GetEventBus().Publish(CarArrivedEvent{ m_DropPos });

                if (!m_ArePackagesCollected)
                {
                    glm::vec3 mushroomWorldPos = {0.0f, 0.0f, 0.0f};
                    auto* mushTransform = GetScene()->GetWorld().GetComponentByID<TransformComponent>(m_MushroomEntity);

                    if (mushTransform && transform) {
                        glm::vec3 localPos = mushTransform->GetPosition();
                        mushroomWorldPos = glm::vec3(transform->GetLocalMatrix() * glm::vec4(localPos, 1.0f));
                    }

                    GetScene()->GetWorld().GetEventBus().Publish(DeliveryMushroomAppearedEvent{ mushroomWorldPos });
                }
                spdlog::info("[DeliveryCar] Klapa w pełni otwarta. Grzyb na pozycji.");
            }
            break;
        }

        case DeliveryState::DROPPING:
        {
            shouldPlayEngine = true;
            engineVolume = 0.05f;

            if (m_ArePackagesCollected)
            {

                if (animator) {
                    animator->IsPlaying = true;
                    animator->PlaybackSpeed = 0.417f; // bo 15 klatek w blenderze
                    animator->PlayAnimation("Close", true, false);
                }

                m_State = DeliveryState::ANIMATING_CLOSE;
                m_AnimationTimer = 1.5f;
                spdlog::info("[DeliveryCar] Paczki odebrane! Zamykam klape.");
            }
            break;
        }

        case DeliveryState::ANIMATING_CLOSE:
        {
            shouldPlayEngine = true;
            float progress = 1.0f - (m_AnimationTimer / 1.5f);
            progress = glm::clamp(progress, 0.0f, 1.0f);
            engineVolume = glm::mix(0.05f, 0.2f, progress);

            m_AnimationTimer -= (float)ts;

            if (m_MushroomSpawned) {
                auto* mushTransform = GetScene()->GetWorld().GetComponentByID<TransformComponent>(m_MushroomEntity);
                if (mushTransform) {
                    float progress = 1.0f - (m_AnimationTimer / 1.5f);
                    progress = glm::clamp(progress, 0.0f, 1.0f);

                    float currentY = glm::mix(poppedY, hiddenY, progress);
                    mushTransform->SetPosition({xOffset, currentY, zOffset});
                }
            }

            if (m_AnimationTimer <= 0.0f)
            {
                m_MushroomSpawned = false;

                m_State = DeliveryState::DRIVING_OUT;
                if (animator) animator->IsPlaying = false;
                spdlog::info("[DeliveryCar] Klapa zamknieta. Odjeżdzam!");
            }
            break;
        }

        case DeliveryState::DRIVING_OUT:
        {
            shouldPlayEngine = true;
            engineVolume = 0.2f;

            glm::vec3 dir = m_ExitPos - currentPos;
            float dist = glm::length(dir);
            float stepDistance = m_Speed * (float)ts.GetSeconds();

            if (dist < stepDistance)
            {
                spdlog::info("Dostawczak opuścił mapę. Niszczenie encji...");
                GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_Entity });
            }
            else
            {
                dir = glm::normalize(dir);
                currentPos += dir * m_Speed * (float)ts.GetSeconds();
                transform->SetPosition(currentPos);
            }
            break;
        }
    }

    if (m_EngineSound) {
        if (shouldPlayEngine && AudioEngine::AreSoundsEnabled()) {

            ma_sound_set_volume(m_EngineSound, engineVolume);

            if (!m_EngineSoundStarted) {
                AudioEngine::ReplaySound(m_EngineSound);
                m_EngineSoundStarted = true;
            }
        } else {
            ma_sound_set_volume(m_EngineSound, 0.0f);

            if (m_EngineSoundStarted) {
                ma_sound_stop(m_EngineSound);
                m_EngineSoundStarted = false;
            }
        }
    }
}