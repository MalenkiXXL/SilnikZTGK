#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include <string>

class GameManagerScript : public ScriptableEntity
{
public:
    int m_CurrentIngredients = 0;
    bool m_IsDeliveryOnTheWay = false;

    std::string m_VanPrefabPath = "CookingStation/Assets/prefabs/deliveryCar.json";

    void OnCreate() override;
    void OnUpdate(Timestep ts) override;

    void AddIngredients(int amount);
    void UseIngredient();

private:
    void CallForDelivery();
};