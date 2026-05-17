#include "CookingStation/Scripts/GameManagerScript.h"
#include "CookingStation/Scripts/DeliveryCarScript.h"
#include "CookingStation/Scene/PrefabSerializer.h"
#include <spdlog/spdlog.h>

void GameManagerScript::OnCreate()
{
    s_Instance = this;
    spdlog::info("GameManager uruchomiony!");
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
    glm::vec3 startPos = DeliveryCarScript::m_StartPos;
    PrefabSerializer::Deserialize(GetScene(), m_VanPrefabPath, startPos);

    m_IsDeliveryOnTheWay = true;
    spdlog::info("GameManager: Brak składników! Wysłano nowego dostawczaka.");
}