#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scene/ecs.h" 
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Core/AudioEngine.h"
#include <string>
#include <vector>
#include <spdlog/spdlog.h>
#include <random>

enum class CustomerState { Spawning, WalkingToChair, Seated };

class CustomerScript : public ScriptableEntity
{
public:
    static inline Entity s_GrandmaTargetChair = { std::numeric_limits<std::size_t>::max(), 0 };
    static inline glm::vec3 s_GrandmaTargetPos = { 0.0f, 0.0f, 0.0f };
    static inline glm::vec3 s_GrandmaFinalRotation = { 0.0f, 0.0f, 0.0f };

    CustomerState State = CustomerState::Spawning;
    bool ReachedWaypoint = false;

    Entity TargetChair = { std::numeric_limits<std::size_t>::max(), 0 };
    glm::vec3 TargetPos = { 0.0f, 0.0f, 0.0f };
    glm::vec3 FinalRotation = { 0.0f, 0.0f, 0.0f };
    bool IsGrandma = false;

    std::size_t m_ValidationResponseSubId = 0;
    bool IsPendingDestroy = false;
    IngredientType WantedIngredient = IngredientType::None;
    bool IsServed = false;
    bool OrderTaken = false;

    Entity m_ReceivedFood = { std::numeric_limits<std::size_t>::max(), 0 };

    std::size_t m_ServedSubId = 0;
    std::size_t m_OrderSubId = 0;
    float OrderPrice = 50.0f;

    void OnCreate() override
    {
        auto* tagComp = GetComponent<TagComponent>();
        IsGrandma = (tagComp && tagComp->Tag == "GrandmaCustomer");

        std::vector<IngredientType> menu = {
            IngredientType::Tomato, IngredientType::Cheese,
            IngredientType::Ham, IngredientType::Sandwich
        };

        std::random_device rd;
        std::mt19937 gen(rd());

        if (IsGrandma) {
            WantedIngredient = IngredientType::Sandwich;
            State = CustomerState::WalkingToChair;
            TargetChair = s_GrandmaTargetChair;
            TargetPos = s_GrandmaTargetPos;
            FinalRotation = s_GrandmaFinalRotation;
        }
        else {
            std::uniform_int_distribution<> dist(0, (int)menu.size() - 1);
            WantedIngredient = menu[dist(gen)];
            State = CustomerState::Seated;
        }

        OrderTaken = false;
        std::vector<float> prices = { 25.0f, 50.0f, 75.0f };
        std::uniform_int_distribution<> priceDist(0, (int)prices.size() - 1);
        OrderPrice = prices[priceDist(gen)];

        spdlog::info("Klient nr {} usiadl/zmierza do stolika. Czeka na: {}", m_Entity.id, IngredientTypeToString(WantedIngredient));

        auto& bus = GetScene()->GetWorld().GetEventBus();

        if (!IsGrandma) {
            bus.Publish(CustomerSeatedEvent{ m_Entity });
        }

        m_ServedSubId = bus.Subscribe<CustomerServedEvent>([this](const CustomerServedEvent& e) {
            if (e.Customer.id == m_Entity.id) {
                m_ReceivedFood = e.ServedFood;
                GetScene()->GetWorld().GetEventBus().Publish(ValidateOrderRequestEvent{
                    m_Entity, e.ServedFood, WantedIngredient
                    });
            }
            });

        m_ValidationResponseSubId = bus.Subscribe<ValidateOrderResponseEvent>([this](const ValidateOrderResponseEvent& e) {
            if (e.Customer.id == m_Entity.id) {
                this->ReceiveFood(e.IsCorrect);
            }
            });

        m_OrderSubId = bus.Subscribe<OrderTakenEvent>([this](const OrderTakenEvent& e) {
            if (e.Customer.id == m_Entity.id) {
                this->OrderTaken = true;
                GetScene()->GetWorld().GetEventBus().Publish(KitchenOrderPlacedEvent{ m_Entity, WantedIngredient });
            }
            });
    }

    void OnUpdate(Timestep ts) override
    {
        if (State == CustomerState::WalkingToChair)
        {
            auto* tf = GetComponent<TransformComponent>();
            if (!tf) return;

            glm::vec3 pos = tf->GetPosition();

            glm::vec3 currentTarget = ReachedWaypoint ? TargetPos : glm::vec3(-9.0f, TargetPos.y, -37.0f);

            glm::vec3 dir = currentTarget - pos;
            dir.y = 0.0f;

            float dist = glm::length(dir);
            float speed = 1.4f;
            float step = speed * (float)ts.GetSeconds();

            if (dist <= step || dist < 0.8f)
            {
                if (!ReachedWaypoint) {
                    ReachedWaypoint = true;
                    pos.x = currentTarget.x;
                    pos.z = currentTarget.z;
                    tf->SetPosition(pos);
                }
                else {
                    tf->SetPosition(TargetPos);
                    tf->SetRotation(FinalRotation);
                    State = CustomerState::Seated;

                    s_GrandmaTargetChair = { std::numeric_limits<std::size_t>::max(), 0 };

                    auto* animator = GetComponent<AnimatorComponent>();
                    if (animator && animator->AnimatorInstance) {
                        animator->AnimatorInstance->PlayAnimation("SitIdle");
                    }

                    auto& bus = GetScene()->GetWorld().GetEventBus();
                    bus.Publish(CustomerSeatedEvent{ m_Entity });
                    bus.Publish(TriggerHighlightEvent{ m_Entity, { 1.0f, 0.8f, 0.2f }, 0.0f, true });
                }
            }
            else
            {
                dir = glm::normalize(dir);
                pos += dir * step;
                tf->SetPosition(pos);

                float angle = glm::degrees(std::atan2(dir.x, dir.z));
                tf->SetRotation({ 0.0f, angle, 0.0f });
            }
        }
    }

    void OnDestroy() override
    {
        auto* scene = GetScene();
        if (scene) {
            auto& bus = scene->GetWorld().GetEventBus();
            if (m_ServedSubId != 0) bus.Unsubscribe<CustomerServedEvent>(m_ServedSubId);
            if (m_OrderSubId != 0) bus.Unsubscribe<OrderTakenEvent>(m_OrderSubId);
            if (m_ValidationResponseSubId != 0) bus.Unsubscribe<ValidateOrderResponseEvent>(m_ValidationResponseSubId);
        }
        if (IsGrandma && State == CustomerState::WalkingToChair) {
            s_GrandmaTargetChair = { std::numeric_limits<std::size_t>::max(), 0 };
        }
    }

    bool IsOrderMatching(const std::vector<IngredientType>& ingredientsOnPlate)
    {
        if (ingredientsOnPlate.empty()) return false;
        for (const auto& item : ingredientsOnPlate)
        {
            if (item == WantedIngredient) return true;
        }
        return false;
    }

    virtual void ReceiveFood(bool isCorrectOrder = true)
    {
        if (IsPendingDestroy) return;
        IsPendingDestroy = true;
        IsServed = true;

        if (m_ReceivedFood.id != std::numeric_limits<std::size_t>::max()) {
            GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_ReceivedFood });
            m_ReceivedFood = { std::numeric_limits<std::size_t>::max(), 0 };
        }

        if (isCorrectOrder)
        {
            spdlog::info("Klient nr {} dostal to, czego chcial!", m_Entity.id);
            if (GameManagerScript::s_Instance)
            {
                OrderFulfilledEvent e(OrderPrice);
                GetScene()->GetWorld().GetEventBus().Publish(e);
            }
            auto* tag = GetComponent<TagComponent>();
            if (tag) tag->Tag = "ZadowolonyKlient";
        }
        else
        {
            spdlog::info("Klient nr {} dostal puste/zle zamowienie!", m_Entity.id);
            auto* tag = GetComponent<TagComponent>();
            if (tag) tag->Tag = "ZlyKlient";
        }

        GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_Entity });
    }
};