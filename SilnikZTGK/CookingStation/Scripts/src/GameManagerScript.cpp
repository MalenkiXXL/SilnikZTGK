#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include "CookingStation/Scripts/Delivery/DeliveryCarScript.h"
#include "CookingStation/Scene/PrefabSerializer.h"
#include <spdlog/spdlog.h>

void GameManagerScript::OnCreate()
{
    s_Instance = this;
    spdlog::info("GameManager uruchomiony!");

    m_IngredientUsedSubId = GetScene()->GetWorld().GetEventBus().Subscribe<IngredientUsedEvent>(
        [this](const IngredientUsedEvent& e) {
            this->UseIngredient(e.Type, e.Amount);
        }
    );

    m_AddIngredientSubId = GetScene()->GetWorld().GetEventBus().Subscribe<AddIngredientEvent>(
        [this](const AddIngredientEvent& e) {
            this->AddIngredients(e.Type, e.Amount);
        }
    );

    m_OrderFulfilledSubId = GetScene()->GetWorld().GetEventBus().Subscribe<OrderFulfilledEvent>(
        [this](const OrderFulfilledEvent& e) {
            this->OnOrderFulfilled(e);
        }
    );
}

void GameManagerScript::OnUpdate(Timestep ts)
{
    if (GetIngredientCount(IngredientType::Tomato) <= 0 && !m_IsDeliveryOnTheWay)
    {
        CallForDelivery();
    }
}

void GameManagerScript::OnDestroy()
{
    GetScene()->GetWorld().GetEventBus().Unsubscribe<IngredientUsedEvent>(m_IngredientUsedSubId);
    GetScene()->GetWorld().GetEventBus().Unsubscribe<AddIngredientEvent>(m_AddIngredientSubId);
    GetScene()->GetWorld().GetEventBus().Unsubscribe<OrderFulfilledEvent>(m_OrderFulfilledSubId);

    s_Instance = nullptr;
}

void GameManagerScript::AddIngredients(IngredientType type, int amount)
{
    m_Inventory[type] += amount;

    InventoryChangedEvent e;
    e.Type = type;
    e.NewAmount = m_Inventory[type];
    GetScene()->GetWorld().GetEventBus().Publish(e);

    spdlog::info("GameManager: Wysłano InventoryChangedEvent dla {} ilość: {}", (int)type, e.NewAmount);
}

void GameManagerScript::UseIngredient(IngredientType type, int amount)
{
    if (m_Inventory[type] >= amount)
    {
        m_Inventory[type] -= amount;

        GetScene()->GetWorld().GetEventBus().Publish(InventoryChangedEvent{ type, m_Inventory[type] });
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
    money += amount;
    GetScene()->GetWorld().GetEventBus().Publish(MoneyChangedEvent{ money });
    return true;
}

bool GameManagerScript::SpendMoney(int amount) {
    if (money >= amount) {
        money -= amount;
        GetScene()->GetWorld().GetEventBus().Publish(MoneyChangedEvent{ money });
        return true;
    }
    return false;
}

void GameManagerScript::OnOrderFulfilled(const OrderFulfilledEvent& e)
{
    AddMoney(static_cast<int>(e.RewardAmount));
    spdlog::info("Order fulfilled! Reward added: {}", e.RewardAmount);
}