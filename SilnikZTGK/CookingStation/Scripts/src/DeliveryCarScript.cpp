#include "CookingStation/Scripts/Delivery/DeliveryCarScript.h"
#include "CookingStation/Scene/PrefabSerializer.h"
#include <spdlog/spdlog.h>

void DeliveryCarScript::OnCreate()
{
    m_CollectedSubId = GetScene()->GetWorld().GetEventBus().Subscribe<DeliveryCollectedEvent>(
            [this](const DeliveryCollectedEvent& e) {
                this->m_ArePackagesCollected = true;
            }
    );
}

void DeliveryCarScript::OnDestroy()
{
    GetScene()->GetWorld().GetEventBus().Unsubscribe<DeliveryCollectedEvent>(m_CollectedSubId);
}

void DeliveryCarScript::OnUpdate(Timestep ts)
{
    auto* transform = GetComponent<TransformComponent>();
    auto* animator = GetComponent<AnimatorComponent>();
    if (!transform) return;

    glm::vec3 currentPos = transform->GetPosition();

    float hiddenY = -0.5f;   // Jak nisko grzyb jest w bagażniku (żeby go nie było widać)
    float poppedY = 1.0f;    // Jak wysoko wyskakuje, żeby było go widać
    float zOffset = -2.5f;   // Przesunięcie do tyłu (do bagażnika) vana
    float xOffset = 0.0f;    // Przesunięcie lewo/prawo (środek)

    switch (m_State)
    {
        case DeliveryState::DRIVING_IN:
        {
            glm::vec3 dir = m_DropPos - currentPos;
            float dist = glm::length(dir);

            if (dist < 0.1f)
            {
                transform->SetPosition(m_DropPos);

                if (animator && animator->AnimatorInstance) {
                    animator->IsPlaying = true;
                    animator->AnimatorInstance->PlayAnimation("Open", true);
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
            m_AnimationTimer -= (float)ts;

            if (!m_MushroomSpawned) {
                m_MushroomEntity = PrefabSerializer::Deserialize(GetScene(), "assets://prefabs/deliveryMushroom.json", m_DropPos);

                GetScene()->SetParent(m_MushroomEntity, m_Entity);

                m_MushroomSpawned = true;
                GetScene()->GetWorld().GetEventBus().Publish(CarArrivedEvent{ m_DropPos });
            }

            if (m_MushroomSpawned) {
                auto* mushTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_MushroomEntity);
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
                spdlog::info("[DeliveryCar] Klapa w pełni otwarta. Grzyb na pozycji.");
            }
            break;
        }

        case DeliveryState::DROPPING:
        {
            if (m_ArePackagesCollected)
            {

                if (animator && animator->AnimatorInstance) {
                    animator->IsPlaying = true;
                    animator->AnimatorInstance->PlayAnimation("Close", true);
                }

                m_State = DeliveryState::ANIMATING_CLOSE;
                m_AnimationTimer = 1.5f;
                spdlog::info("[DeliveryCar] Paczki odebrane! Zamykam klape.");
            }
            break;
        }

        case DeliveryState::ANIMATING_CLOSE:
        {
            m_AnimationTimer -= (float)ts;

            if (m_MushroomSpawned) {
                auto* mushTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_MushroomEntity);
                if (mushTransform) {
                    float progress = 1.0f - (m_AnimationTimer / 1.5f);
                    progress = glm::clamp(progress, 0.0f, 1.0f);

                    float currentY = glm::mix(poppedY, hiddenY, progress);
                    mushTransform->SetPosition({xOffset, currentY, zOffset});
                }
            }

            if (m_AnimationTimer <= 0.0f)
            {
                if (m_MushroomSpawned)
                {
                    GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_MushroomEntity });
                    m_MushroomEntity = { std::numeric_limits<std::size_t>::max(), 0 };
                    m_MushroomSpawned = false;
                }

                m_State = DeliveryState::DRIVING_OUT;
                if (animator) animator->IsPlaying = false;
                spdlog::info("[DeliveryCar] Klapa zamknieta, grzyb usunięty. Odjeżdzam!");
            }
            break;
        }

        case DeliveryState::DRIVING_OUT:
        {
            glm::vec3 dir = m_ExitPos - currentPos;
            float dist = glm::length(dir);

            if (dist < 0.1f)
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
}
