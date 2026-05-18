#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scripts/Managers/IngredientType.h"
#include <string>

class GameManagerScript : public ScriptableEntity
{
public:
    inline static GameManagerScript* s_Instance = nullptr;

    std::unordered_map<IngredientType, int> m_Inventory;
    bool m_IsDeliveryOnTheWay = false;

    std::string m_VanPrefabPath = "CookingStation/Assets/prefabs/deliveryCar.json";

    void OnCreate() override;
    void OnUpdate(Timestep ts) override;

    void AddIngredients(IngredientType type, int amount);

    void UseIngredient(IngredientType type, int amount);

    int GetIngredientCount(IngredientType type);

    void OnDestroy() override
    {
        s_Instance = nullptr;
    }

private:
    void CallForDelivery();


};