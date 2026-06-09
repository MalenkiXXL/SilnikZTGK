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

    static std::vector<IngredientType> CalculateWhatToOrder(
            const std::vector<OrderRecord>& activeOrders,
            std::map<IngredientType, int> currentInventory,
            const std::map<IngredientType, int>& minThresholds)
    {
        std::vector<IngredientType> selectedIngredients;

        // --- WĘZEŁ 1: Priorytet - Zamówienia klientów ---
        for (const auto& order : activeOrders) {
            IngredientType neededType = order.WantedDish;
            if (currentInventory[neededType] > 0) {
                currentInventory[neededType]--;
            } else {
                // Sprawdzamy, czy tego składnika jeszcze nie zamówiliśmy
                if (std::find(selectedIngredients.begin(), selectedIngredients.end(), neededType) == selectedIngredients.end()) {
                    selectedIngredients.push_back(neededType);
                    if (selectedIngredients.size() == 2) return selectedIngredients;
                }
            }
        }

        // --- WĘZEŁ 2: Uzupełnianie zapasów wg progów minimalnych ---
        for (const auto& [type, threshold] : minThresholds) {
            if (currentInventory[type] < threshold) {
                if (std::find(selectedIngredients.begin(), selectedIngredients.end(), type) == selectedIngredients.end()) {
                    selectedIngredients.push_back(type);
                    if (selectedIngredients.size() == 2) return selectedIngredients;
                }
            }
        }

        // Jeśli nikt nic nie zamawia i niczego nie brakuje
        if (selectedIngredients.empty()) {
            return { IngredientType::None };
        }

        // --- WĘZEŁ 3: Wypełniacz (zawsze dajemy graczowi 2 opcje) ---
        for (const auto& [type, threshold] : minThresholds) {
            if (std::find(selectedIngredients.begin(), selectedIngredients.end(), type) == selectedIngredients.end()) {
                selectedIngredients.push_back(type);
                if (selectedIngredients.size() == 2) return selectedIngredients;
            }
        }

        return selectedIngredients;
    }
};