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

    // Zapisujemy ID subskrypcji, by móc się wyrejestrować przy zniszczeniu
    std::size_t m_ServedSubId = 0;
    std::size_t m_OrderSubId = 0;

   void OnCreate() override
    {
        // 1. Definiujemy, co jest w menu (możesz tu dodawać kolejne stringi, byle odpowiadały systemowi potraw)
        std::vector<std::string> menu = { "Tomato", "Cheese", "Ham", "Sandwich"};

        // 2. Losujemy jeden ze składników
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, menu.size() - 1);

        WantedIngredient = menu[dist(gen)];
        OrderTaken = false;
        
        spdlog::info("Klient nr {} usiadl i czeka na: {}", m_Entity.id, WantedIngredient);

        auto& bus = GetScene()->GetWorld().GetEventBus();

        // 1. Publikujemy info do kelnerów, że usiedliśmy
        bus.Publish(CustomerSeatedEvent{ m_Entity });

        // 2. Nasłuchujemy, czy kelner przyniósł nam jedzenie
        m_ServedSubId = bus.Subscribe<CustomerServedEvent>([this](const CustomerServedEvent& e) {
            if (e.Customer.id == m_Entity.id) {
                // Klient sam ocenia, czy to co dostał to jest to, co chciał!
                bool isCorrect = this->IsOrderMatching(e.ServedIngredients);
                this->ReceiveFood(isCorrect);
            }
            });

        // 3. Nasłuchujemy, czy kelner spisał już nasze zamówienie
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