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

        spdlog::info("--- AI DOSTAW: Rozpoczynam analize potrzeb ---");

        // --- WĘZEŁ 1: Priorytet - Zamówienia klientów ---
        for (const auto& order : activeOrders) {
            IngredientType neededType = order.WantedDish;
            if (currentInventory[neededType] > 0) {
                currentInventory[neededType]--;
                spdlog::debug("[WEZEL 1] Klient {} chce danie ({}). Mamy na stanie, rezerwuje sztuke.",
                              order.CustomerId, IngredientTypeToString(neededType));
            } else {
                if (std::find(selectedIngredients.begin(), selectedIngredients.end(), neededType) == selectedIngredients.end()) {
                    selectedIngredients.push_back(neededType);
                    spdlog::info("[WEZEL 1] Klient {} chce danie ({}). BRAKI W MAGAZYNIE! Dodaje do listy zakupow.",
                                 order.CustomerId, IngredientTypeToString(neededType));
                    if (selectedIngredients.size() == 2) {
                        spdlog::info("AI DOSTAW: Koszyk pelny (2/2). Koncze analize.");
                        return selectedIngredients;
                    }
                }
            }
        }

        // --- WĘZEŁ 2: Uzupełnianie zapasów wg progów minimalnych ---
        for (const auto& [type, threshold] : minThresholds) {
            if (currentInventory[type] < threshold) {
                if (std::find(selectedIngredients.begin(), selectedIngredients.end(), type) == selectedIngredients.end()) {
                    selectedIngredients.push_back(type);
                    spdlog::info("[WEZEL 2] Zapasy skladnika ({}) spadly ponizej progu ({}). Dodaje do listy.",
                                 IngredientTypeToString(type), threshold);
                    if (selectedIngredients.size() == 2) {
                        spdlog::info("AI DOSTAW: Koszyk pelny (2/2). Koncze analize.");
                        return selectedIngredients;
                    }
                }
            }
        }

        // Jeśli nikt nic nie zamawia i niczego nie brakuje
        if (selectedIngredients.empty()) {
            spdlog::info("[WEZEL 3] Brak brakow. Nie zamawiam nic (None).");
            return { IngredientType::None };
        }

        // --- WĘZEŁ 3: Wypełniacz (zawsze dajemy graczowi 2 opcje) ---
        for (const auto& [type, threshold] : minThresholds) {
            if (std::find(selectedIngredients.begin(), selectedIngredients.end(), type) == selectedIngredients.end()) {
                selectedIngredients.push_back(type);
                spdlog::info("[WEZEL 3] Uzupelniam wolne miejsce w dostawie losowym skladnikiem ({}).",
                             IngredientTypeToString(type));
                if (selectedIngredients.size() == 2) {
                    spdlog::info("AI DOSTAW: Koszyk dopelniony (2/2). Koncze analize.");
                    return selectedIngredients;
                }
            }
        }

        return selectedIngredients;
    }
};