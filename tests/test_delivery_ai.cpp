#include <gtest/gtest.h>
#include "../SilnikZTGK/CookingStation/Scripts/Delivery/DeliveryLogic.h"

/**
 * Zestaw testów jednostkowych dla modułu DeliveryLogic.
 * Testy weryfikują poprawność działania algorytmu decyzyjnego magazynu,
 * w tym priorytetyzację zamówień, symulację zużycia (wirtualny magazyn)
 * oraz obsługę progów minimalnych.
 */

// ---------------------------------------------------------
// TEST 1: Weryfikacja stanu spoczynku (Brak akcji)
// ---------------------------------------------------------
TEST(DeliveryLogicTest, NoOrdersAndPantryFull_ReturnsNone) {
    // Arrange (Przygotowanie danych)
    std::vector<OrderRecord> queue = {};
    std::map<IngredientType, int> inventory = { { IngredientType::Tomato, 5 } };
    std::map<IngredientType, int> thresholds = { { IngredientType::Tomato, 3 } };

    // Act (Wykonanie logiki biznesowej)
    IngredientType result = DeliveryLogic::CalculateWhatToOrder(queue, inventory, thresholds);

    // Assert (Weryfikacja wyniku)
    // Stan zapasów jest stabilny, a kolejka pusta. System nie powinien generować zamówień.
    EXPECT_EQ(result, IngredientType::None);
}

// ---------------------------------------------------------
// TEST 2: Priorytetyzacja najstarszego zamówienia w kolejce
// ---------------------------------------------------------
TEST(DeliveryLogicTest, PrioritizesCorrectCustomerInQueue) {
    // Arrange
    std::vector<OrderRecord> queue = {
            { 101, IngredientType::Ham },     // Klient 1 (Najwyższy priorytet)
            { 102, IngredientType::Cheese }   // Klient 2
    };

    std::map<IngredientType, int> inventory = {
            { IngredientType::Ham, 1 },       // Stan wystarczający dla Klienta 1
            { IngredientType::Cheese, 0 }     // Brak asortymentu dla Klienta 2
    };
    std::map<IngredientType, int> thresholds = {};

    // Act
    IngredientType result = DeliveryLogic::CalculateWhatToOrder(queue, inventory, thresholds);

    // Assert
    // Algorytm powinien zidentyfikować, że wymogi Klienta 1 są spełnione,
    // i zgłosić zapotrzebowanie na składnik dla Klienta 2.
    EXPECT_EQ(result, IngredientType::Cheese);
}

// ---------------------------------------------------------
// TEST 3: Weryfikacja mechanizmu wirtualnego zużycia zapasów
// ---------------------------------------------------------
TEST(DeliveryLogicTest, MultipleCustomersSameIngredient_DetectsShortage) {
    // Arrange
    std::vector<OrderRecord> queue = {
            { 101, IngredientType::Tomato },
            { 102, IngredientType::Tomato }
    };
    std::map<IngredientType, int> inventory = {
            { IngredientType::Tomato, 1 } // Dostępna tylko 1 sztuka
    };
    std::map<IngredientType, int> thresholds = {};

    // Act
    IngredientType result = DeliveryLogic::CalculateWhatToOrder(queue, inventory, thresholds);

    // Assert
    // Pierwszy klient wirtualnie konsumuje jedynego pomidora.
    // Dla drugiego klienta algorytm musi wykryć brak i wygenerować zamówienie.
    EXPECT_EQ(result, IngredientType::Tomato);
}

// ---------------------------------------------------------
// TEST 4: Uzupełnianie braków strukturalnych (Poniżej progu)
// ---------------------------------------------------------
TEST(DeliveryLogicTest, RefillsEmptyPantryWhenOrdersAreCovered) {
    // Arrange
    std::vector<OrderRecord> queue = {
            { 101, IngredientType::Tomato }
    };

    std::map<IngredientType, int> inventory = {
            { IngredientType::Tomato, 1 }, // Zabezpiecza bieżące zamówienie
            { IngredientType::Milk, 1 }    // Poniżej progu minimalnego!
    };
    std::map<IngredientType, int> thresholds = {
            { IngredientType::Milk, 3 }    // Wymagane minimum to 3 sztuki
    };

    // Act
    IngredientType result = DeliveryLogic::CalculateWhatToOrder(queue, inventory, thresholds);

    // Assert
    // Skoro zamówienia klientów są zabezpieczone wirtualnym magazynem,
    // algorytm przechodzi do uzupełniania braków ogólnych w spiżarni.
    EXPECT_EQ(result, IngredientType::Milk);
}

// ---------------------------------------------------------
// TEST 5: Zmiana stanu po obsłużeniu klienta (Przesunięcie priorytetów)
// ---------------------------------------------------------
TEST(DeliveryLogicTest, CustomerServed_UpdatesPriorities) {
    // Arrange
    // Symulacja stanu po wydaniu posiłku: Klient 101 zniknął z kolejki,
    // a gracz właśnie zużył ostatnią szynkę na jego zamówienie.
    std::vector<OrderRecord> queueAfterServing = {
            { 102, IngredientType::Cheese } // Oczekuje tylko Klient 102
    };

    std::map<IngredientType, int> inventoryAfterServing = {
            { IngredientType::Ham, 0 },    // Stan po wydaniu posiłku
            { IngredientType::Cheese, 0 }  // Stały brak
    };

    std::map<IngredientType, int> thresholds = {
            { IngredientType::Ham, 3 }     // Szynka drastycznie poniżej progu
    };

    // Act
    IngredientType result = DeliveryLogic::CalculateWhatToOrder(queueAfterServing, inventoryAfterServing, thresholds);

    // Assert
    // Pomimo krytycznego braku szynki (0 względem 3 wymaganych),
    // priorytetem pozostaje aktywna kolejka zamówień. Algorytm musi zamówić ser.
    EXPECT_EQ(result, IngredientType::Cheese);
}