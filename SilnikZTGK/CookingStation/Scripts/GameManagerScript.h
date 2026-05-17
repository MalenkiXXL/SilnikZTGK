#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scripts/DeliveryCarScript.h"
#include <spdlog/spdlog.h>

class GameManagerScript : public ScriptableEntity
{
public:
    int m_CurrentIngredients = 0; // Ilość składników w bazie/na stole
    bool m_IsDeliveryOnTheWay = false;

    void OnCreate() override
    {
        // Manager na start ma np. 0 składników, więc od razu wezwie dostawę
        spdlog::info("GameManager uruchomiony!");
    }

    void OnUpdate(Timestep ts) override
    {
        // Sprawdzamy co klatkę stan gry
        if (m_CurrentIngredients <= 0 && !m_IsDeliveryOnTheWay)
        {
            CallForDelivery();
        }
    }

    // Wywołaj tę funkcję ze skryptu paczki, gdy Gracz ją podniesie!
    void AddIngredients(int amount)
    {
        m_CurrentIngredients += amount;
        m_IsDeliveryOnTheWay = false; // Resetujemy stan dostawy
        spdlog::info("Dodano składniki! Obecnie: {}", m_CurrentIngredients);
    }

    // Wywołaj to ze skryptu np. stanowiska do gotowania, gdy gracz weźmie składnik
    void UseIngredient()
    {
        if (m_CurrentIngredients > 0)
        {
            m_CurrentIngredients--;
            spdlog::info("Zużyto składnik! Zostało: {}", m_CurrentIngredients);
        }
    }

private:
    void CallForDelivery()
    {
        // Sprawdzamy czy dostawczak w ogóle istnieje na scenie
        if (DeliveryCarScript::s_Instance != nullptr)
        {
            if (DeliveryCarScript::s_Instance->m_State == DeliveryState::IDLE)
            {
                DeliveryCarScript::s_Instance->NeedsDelivery = true;
                m_IsDeliveryOnTheWay = true;
                spdlog::info("GameManager: Brak składników! Zamówiono dostawę.");
            }
        }
        else
        {
            spdlog::warn("GameManager: Chciano zamówić dostawę, ale nie znaleziono skryptu Dostawczaka!");
        }
    }
};