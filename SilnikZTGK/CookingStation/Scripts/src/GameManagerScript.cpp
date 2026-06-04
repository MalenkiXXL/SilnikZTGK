#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include "CookingStation/Scene/PrefabSerializer.h"
#include <spdlog/spdlog.h>

void GameManagerScript::OnCreate()
{
    s_Instance = this;
    spdlog::info("GameManager uruchomiony!");

    auto& bus = GetScene()->GetWorld().GetEventBus();

    m_IngredientUsedSubId = bus.Subscribe<IngredientUsedEvent>(
        [this](const IngredientUsedEvent& e) {
            this->UseIngredient(e.Type, e.Amount);
        }
    );

    m_AddIngredientSubId = bus.Subscribe<AddIngredientEvent>(
        [this](const AddIngredientEvent& e) {
            this->AddIngredients(e.Type, e.Amount);
        }
    );

    m_OrderFulfilledSubId = bus.Subscribe<OrderFulfilledEvent>(
        [this](const OrderFulfilledEvent& e) {
            this->OnOrderFulfilled(e);
        }
    );

    // NOWE: Rejestrowanie Historii Dań
    m_DishCreatedSubId = bus.Subscribe<DishCreatedEvent>(
        [this](const DishCreatedEvent& e) {
            m_DishMemory[e.FoodEntity.id] = e.History;
        }
    );

    // NOWE: Weryfikacja zamówień przez system
    m_ValidateOrderSubId = bus.Subscribe<ValidateOrderRequestEvent>(
        [this](const ValidateOrderRequestEvent& e) {
            bool isCorrect = false;

            if (m_DishMemory.find(e.ServedFood.id) != m_DishMemory.end()) {
                const auto& history = m_DishMemory[e.ServedFood.id];

                IngredientType wantedType = e.WantedIngredient;

                for (auto ingredient : history.BaseIngredients) {
                    if (ingredient == wantedType ||
                        (wantedType == IngredientType::Tomato && ingredient == IngredientType::ChoppedTomato)) {
                        isCorrect = true;
                        break;
                    }
                }
            }

            GetScene()->GetWorld().GetEventBus().Publish(ValidateOrderResponseEvent{
                    e.Customer,
                    isCorrect
            });
        }
    );

    AddIngredients(IngredientType::Tomato, 5);
}

void GameManagerScript::OnDestroy()
{
    auto& bus = GetScene()->GetWorld().GetEventBus();

    bus.Unsubscribe<IngredientUsedEvent>(m_IngredientUsedSubId);
    bus.Unsubscribe<AddIngredientEvent>(m_AddIngredientSubId);
    bus.Unsubscribe<OrderFulfilledEvent>(m_OrderFulfilledSubId);

    // Zwalnianie nowych eventów!
    bus.Unsubscribe<DishCreatedEvent>(m_DishCreatedSubId);
    bus.Unsubscribe<ValidateOrderRequestEvent>(m_ValidateOrderSubId);

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