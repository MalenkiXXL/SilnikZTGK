#pragma once
#include <vector>
#include <map>
#include "../Managers/IngredientType.h"

struct OrderRecord {
    std::size_t CustomerId;
    IngredientType WantedDish;
};

class DeliveryLogic {
public:

    static IngredientType CalculateWhatToOrder(
            const std::vector<OrderRecord>& activeOrders,
            std::map<IngredientType, int> currentInventory,
            const std::map<IngredientType, int>& minThresholds)
    {
        // 1. Sprawdzamy braki pod konkretne zamówienia klientów
        for (const auto& order : activeOrders) {
            IngredientType neededType = order.WantedDish;
            if (currentInventory[neededType] > 0) {
                currentInventory[neededType]--;
            } else {
                return neededType;
            }
        }

        // 2. Sprawdzamy braki w spiżarni
        for (const auto& [type, threshold] : minThresholds) {
            if (currentInventory[type] < threshold) {
                return type;
            }
        }

        return IngredientType::None;
    }
};