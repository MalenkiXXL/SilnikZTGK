#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include "CookingStation/Scripts/Delivery/DeliveryCarScript.h"
#include "CookingStation/Scene/PrefabSerializer.h"
#include <spdlog/spdlog.h>

void GameManagerScript::OnCreate()
{
    s_Instance = this;
    spdlog::info("GameManager uruchomiony!");
}

void GameManagerScript::OnUpdate(Timestep ts)
{
    if (GetIngredientCount(IngredientType::Tomato) <= 0 && !m_IsDeliveryOnTheWay)
    {
        CallForDelivery();
    }
}

void GameManagerScript::AddIngredients(IngredientType type, int amount)
{
    m_Inventory[type] += amount;
    m_IsDeliveryOnTheWay = false;
    spdlog::info("Dodano składniki! Obecnie: {}",  m_Inventory[type]);
}

void GameManagerScript::UseIngredient(IngredientType type, int amount)
{
    if (m_Inventory[type] >= amount)
    {
        m_Inventory[type] -= amount;
    }
}
int GameManagerScript::GetIngredientCount(IngredientType type)
{
    if (m_Inventory.count(type) > 0)
    {
        return m_Inventory[type];
    }
    return 0;
}


void GameManagerScript::CallForDelivery()
{
    glm::vec3 startPos = DeliveryCarScript::m_StartPos;
    PrefabSerializer::Deserialize(GetScene(), m_VanPrefabPath, startPos);

    m_IsDeliveryOnTheWay = true;
    spdlog::info("GameManager: Brak składników! Wysłano nowego dostawczaka.");
}

int GameManagerScript::GetMoney() {
    return money;
}

bool GameManagerScript::AddMoney(int amount) {
    if (amount > 0){
        money+=amount;
        return true;
    }
    return false;
}

bool GameManagerScript::SpendMoney(int amount) {
    if (amount > 0){
        money-=amount;
        return true;
    }
    return false;
}
