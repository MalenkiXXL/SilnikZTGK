#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scripts/Managers/IngredientType.h"
#include "CookingStation/Scripts/Delivery/DeliveryCarScript.h"
#include "DeliveryLogic.h"
#include <map>
#include <string>
#include <vector>

class DeliveryManagerScript : public ScriptableEntity
{
public:
    void OnCreate() override;
    void OnUpdate(Timestep ts) override;
    void OnDestroy() override;

private:
    void RunDeliveryDecisionTree();
    void CallForDelivery(vector<IngredientType> types);

    std::vector<OrderRecord> m_ActiveOrdersQueue;

    // Pozycja, z której startuje auto
    glm::vec3 m_CarStartPos = DeliveryCarScript::m_StartPos;

    // Przesunięcia dla paczek względem auta
    glm::vec3 m_PackageOffsets[2] = {
            glm::vec3(6.0f, -4.0f, 0.0f),
            glm::vec3(6.0f, -4.0f, 2.0f)
    };

    std::map<IngredientType, int> m_MinThreshold;
    float m_DecisionTimer = 1.0f;
    bool m_IsDeliveryOnTheWay = false;

    std::size_t m_DeliveryCarEntityId = std::numeric_limits<std::size_t>::max();
    std::size_t m_DeliveryDestroySubId = 0;

    std::string m_VanPrefabPath = "CookingStation/Assets/prefabs/deliveryCar.json";
    std::string m_PackagePrefabPath = "CookingStation/Assets/prefabs/package.json";
    std::size_t m_CarArrivedSubId = 0;
    std::size_t m_PackageSpawnedSubId = 0;
    std::size_t m_CustomerSeatedSubId = 0;
    std::size_t m_ValidationResponseSubId = 0;

    std::vector<IngredientType> m_CurrentOrderTypes;
    int m_SpawnedPackagesCount = 0;

};