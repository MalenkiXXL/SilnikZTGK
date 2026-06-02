#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scripts/Managers/IngredientType.h"
#include "CookingStation/Events/GameEvents.h"
#include <string>

class GameManagerScript : public ScriptableEntity
{
public:
    inline static GameManagerScript* s_Instance = nullptr;

    void OnCreate() override;

    void AddIngredients(IngredientType type, int amount);

    void UseIngredient(IngredientType type, int amount);

    int GetIngredientCount(IngredientType type);

    int GetMoney();

    bool AddMoney(int amount);
    bool SpendMoney(int amount);

    void OnDestroy() override;

private:
    int money = 0;
    std::unordered_map<IngredientType, int> m_Inventory;
    std::size_t m_IngredientUsedSubId = 0;
    std::size_t m_AddIngredientSubId = 0;
    std::size_t m_OrderFulfilledSubId = 0;

    void OnOrderFulfilled(const OrderFulfilledEvent& e);

};