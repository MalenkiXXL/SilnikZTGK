#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scene/ecs.h" 
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Core/AudioEngine.h"
#include <string>
#include <vector>
#include <spdlog/spdlog.h>
#include <random>

class CustomerScript : public ScriptableEntity
{
public:
    std::size_t m_ValidationResponseSubId = 0;
    bool IsPendingDestroy = false;
    IngredientType WantedIngredient = IngredientType::None;
    bool IsServed = false;
    bool OrderTaken = false;

    Entity m_ReceivedFood = { std::numeric_limits<std::size_t>::max(), 0 };

    std::size_t m_ServedSubId = 0;
    std::size_t m_OrderSubId = 0;

    void OnCreate() override
    {
        // 1. Definiujemy, co jest w menu (odblokowane pozostałe opcje!)
        std::vector<IngredientType> menu = {
            IngredientType::Tomato,
            IngredientType::Cheese,
            IngredientType::Ham,
            IngredientType::Sandwich
        };

        // 2. Losujemy jeden ze składników
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, menu.size() - 1);

        WantedIngredient = menu[dist(gen)];
        OrderTaken = false;

        spdlog::info("Klient nr {} usiadl i czeka na: {}", m_Entity.id, IngredientTypeToString(WantedIngredient));

        auto& bus = GetScene()->GetWorld().GetEventBus();

        bus.Publish(CustomerSeatedEvent{ m_Entity });

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

                GetScene()->GetWorld().GetEventBus().Publish(KitchenOrderPlacedEvent{
                        m_Entity,
                        WantedIngredient
                });

                spdlog::info("[Customer] Zamówienie klienta {} (Na: {}) wysłane do magazynu!", m_Entity.id, IngredientTypeToString(WantedIngredient));
            }
            });
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
            spdlog::info("Klient nr {} dostal to, czego chcial! Zjada ze smakiem.", m_Entity.id);
            if (GameManagerScript::s_Instance)
            {
                OrderFulfilledEvent e(50.0f);
                GetScene()->GetWorld().GetEventBus().Publish(e);
                spdlog::info("Klient nr {} zaplacil 50 monet!", m_Entity.id);
            }
            auto* tag = GetComponent<TagComponent>();
            if (tag) tag->Tag = "ZadowolonyKlient";
        }
        else
        {
            spdlog::info("Klient nr {} dostal puste/zle zamowienie! Wychodzi bez placenia.", m_Entity.id);
            auto* tag = GetComponent<TagComponent>();
            if (tag) tag->Tag = "ZlyKlient";
        }

        GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_Entity });
        spdlog::info("PUBLISHED DESTROY EVENT");
    }
};