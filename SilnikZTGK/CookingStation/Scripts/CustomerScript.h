#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scene/ecs.h" 
#include "CookingStation/Events/GameEvents.h" 
#include <string>
#include <vector>
#include <spdlog/spdlog.h>
#include <random>

class CustomerScript : public ScriptableEntity
{
public:
    bool IsPendingDestroy = false;
    std::string WantedIngredient = "";
    bool IsServed = false;
    bool OrderTaken = false;

    std::size_t m_ServedSubId = 0;
    std::size_t m_OrderSubId = 0;

    void OnCreate() override
    {
        // 1. Definiujemy, co jest w menu (zakomentowana reszta - zostaje tylko Pomidor)
        std::vector<std::string> menu = { "Tomato" /*, "Cheese", "Ham", "Sandwich"*/ };

        // 2. Losujemy jeden ze składników
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, menu.size() - 1);

        WantedIngredient = menu[dist(gen)];
        OrderTaken = false;

        spdlog::info("Klient nr {} usiadl i czeka na: {}", m_Entity.id, WantedIngredient);

        auto& bus = GetScene()->GetWorld().GetEventBus();

        bus.Publish(CustomerSeatedEvent{ m_Entity });

        m_ServedSubId = bus.Subscribe<CustomerServedEvent>([this](const CustomerServedEvent& e) {
            if (e.Customer.id == m_Entity.id) {
                bool isCorrect = this->IsOrderMatching(e.ServedIngredients);
                this->ReceiveFood(isCorrect);
            }
            });

        m_OrderSubId = bus.Subscribe<OrderTakenEvent>([this](const OrderTakenEvent& e) {
            if (e.Customer.id == m_Entity.id) {
                this->OrderTaken = true;
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
        }
    }

    bool IsOrderMatching(const std::vector<std::string>& ingredientsOnPlate)
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

        if (isCorrectOrder)
        {
            spdlog::info("Klient nr {} dostal to, czego chcial! Zjada ze smakiem.", m_Entity.id);
            if (GameManagerScript::s_Instance)
            {
                // Zapłata w wysokości 50 monet:
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