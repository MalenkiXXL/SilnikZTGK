#include "CookingStation/Scripts/GameManagerScript.h"
#include "CookingStation/Scripts/DeliveryCarScript.h"
#include "CookingStation/Scene/PrefabSerializer.h"
#include <spdlog/spdlog.h>

void GameManagerScript::OnCreate()
{
    spdlog::info("GameManager uruchomiony! Tworzę dostawczaka...");

    glm::vec3 startPos = { -30.0f, 2.0f, 5.0f };
    PrefabSerializer::Deserialize(GetScene(), m_VanPrefabPath, startPos);
}

void GameManagerScript::OnUpdate(Timestep ts)
{
    if (m_CurrentIngredients <= 0 && !m_IsDeliveryOnTheWay)
    {
        CallForDelivery();
    }
}

void GameManagerScript::AddIngredients(int amount)
{
    m_CurrentIngredients += amount;
    m_IsDeliveryOnTheWay = false;
    spdlog::info("Dodano składniki! Obecnie: {}", m_CurrentIngredients);
}

void GameManagerScript::UseIngredient()
{
    if (m_CurrentIngredients > 0)
    {
        m_CurrentIngredients--;
        spdlog::info("Zużyto składnik! Zostało: {}", m_CurrentIngredients);
    }
}

void GameManagerScript::CallForDelivery()
{
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
        spdlog::warn("GameManager: Czekam na wczytanie dostawczaka...");
    }
}