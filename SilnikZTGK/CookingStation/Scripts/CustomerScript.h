#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scene/ecs.h" 
#include "CookingStation/Events/GameEvents.h"
#include <string>
#include <vector>

class CustomerScript : public ScriptableEntity
{
public:
    bool IsPendingDestroy = false;
    std::string WantedIngredient = "";
    bool IsServed = false;

    // NOWE: Flaga sprawdzająca, czy kelner przyjął zamówienie
    bool OrderTaken = false;

    void OnCreate() override
    {
        WantedIngredient = "Tomato";
        OrderTaken = false; // Domyślnie klient oczekuje na kelnera
        spdlog::info("Klient nr {} usiadl i czeka na zlozenie zamowienia", m_Entity.id);
    
        GetScene()->GetWorld().GetEventBus().Publish(CustomerSeatedEvent{ m_Entity });
    }
    // Ta funkcja b�dzie wywo�ywana p�niej przez Kelnera
    bool IsOrderMatching(const std::vector<std::string>& ingredientsOnPlate)
    {
        for (const auto& item : ingredientsOnPlate)
        {
            if (item == WantedIngredient) return true;
        }
        return false;
    }

    virtual void ReceiveFood(bool isCorrectOrder = true)
    {
        if (IsPendingDestroy)
            return;

        IsPendingDestroy = true;

        IsServed = true;

        if (isCorrectOrder)
        {
            spdlog::info("Klient nr {} dostal to, czego chcial! Zjada ze smakiem.", m_Entity.id);
            if (GameManagerScript::s_Instance)
            {
                OrderFulfilledEvent e(50.0f); // 10.0f to nagroda
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