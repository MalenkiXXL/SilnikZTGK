#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scripts/Managers/IngredientType.h"
#include <string>

class GameManagerScript : public ScriptableEntity
{
public:
    inline static GameManagerScript* s_Instance = nullptr;


    bool m_IsDeliveryOnTheWay = false;

    std::string m_VanPrefabPath = "CookingStation/Assets/prefabs/deliveryCar.json";

    void OnCreate() override;
    void OnUpdate(Timestep ts) override;

    void AddIngredients(IngredientType type, int amount);

    void UseIngredient(IngredientType type, int amount);

    int GetIngredientCount(IngredientType type);

    int GetMoney();

    bool AddMoney(int amount);
    bool SpendMoney(int amount);

    void OnDestroy() override
    {
        s_Instance = nullptr;
    }

private:
    int money = 0;
    std::unordered_map<IngredientType, int> m_Inventory;
    void CallForDelivery();


};