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
    if (!transform) return;

    glm::vec3 currentPos = transform->GetPosition();

    switch (m_State)
    {
        case DeliveryState::DRIVING_IN:
        {
            glm::vec3 dir = m_DropPos - currentPos;
            float dist = glm::length(dir);

            if (dist < 0.1f)
            {
                transform->SetPosition(m_DropPos);
                m_State = DeliveryState::DROPPING;

                GetScene()->GetWorld().GetEventBus().Publish(CarArrivedEvent{ m_DropPos });

                spdlog::info("[DeliveryCar] Dojechalem. Czekam na gracza.");
            }
            else
            {
                dir = glm::normalize(dir);
                currentPos += dir * m_Speed * (float)ts.GetSeconds();
                transform->SetPosition(currentPos);
            }
            break;
        }

        case DeliveryState::DROPPING:
        {
            if (m_ArePackagesCollected)
            {
                m_State = DeliveryState::DRIVING_OUT;
                spdlog::info("[DeliveryCar] Paczki odebrane! Wracam do bazy.");
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
