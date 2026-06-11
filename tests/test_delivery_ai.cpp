#include <gtest/gtest.h>
#include "../SilnikZTGK/CookingStation/Scripts/Delivery/DeliveryLogic.h"

/**
 * @file DeliveryLogicTests.cpp
 * @brief Zestaw testów jednostkowych dla modułu DeliveryLogic.
 * * Testy weryfikują poprawność działania algorytmu decyzyjnego magazynu.
 * Główny nacisk położono na weryfikację systemu "dwupakietowego" (algorytm
 * zwraca do dwóch unikalnych składników), priorytetyzację zamówień klientów,
 * symulację zużycia (wirtualny magazyn) oraz zachowanie względem progów minimalnych.
 */

// ---------------------------------------------------------
// TEST 1: Weryfikacja stanu spoczynku (Brak akcji)
// ---------------------------------------------------------
TEST(DeliveryLogicTest, NoOrdersAndPantryFull_ReturnsNone) {
    // Arrange: Konfiguracja stabilnego stanu systemu
    std::vector<OrderRecord> queue = {};
    std::map<IngredientType, int> inventory = { { IngredientType::Tomato, 5 } };
    std::map<IngredientType, int> thresholds = { { IngredientType::Tomato, 3 } };

    // Act: Wywołanie logiki decyzyjnej
    auto result = DeliveryLogic::CalculateWhatToOrder(queue, inventory, thresholds);

    // Assert: System nie powinien generować zbędnych zamówień.
    // Oczekujemy jednoelementowego wektora ze statusem "None".
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], IngredientType::None);
}

// ---------------------------------------------------------
// TEST 2: Priorytetyzacja najstarszego zamówienia oraz dobór dopełnienia
// ---------------------------------------------------------
TEST(DeliveryLogicTest, PrioritizesCorrectCustomerInQueue_AndAddsFiller) {
    // Arrange: Przygotowanie kolejki z klientami o różnym priorytecie
    std::vector<OrderRecord> queue = {
            { 101, IngredientType::Ham },     // Klient 1 (Najwyższy priorytet czasowy)
            { 102, IngredientType::Cheese }   // Klient 2
    };

    std::map<IngredientType, int> inventory = {
            { IngredientType::Ham, 1 },       // Stan wystarczający dla Klienta 1
            { IngredientType::Cheese, 0 }     // Krytyczny brak dla Klienta 2
    };

    // Dodajemy próg, aby algorytm miał z czego dobrać drugą, unikalną paczkę
    std::map<IngredientType, int> thresholds = {
            { IngredientType::Milk, 2 }
    };

    // Act
    auto result = DeliveryLogic::CalculateWhatToOrder(queue, inventory, thresholds);

    // Assert: Weryfikacja systemu dwupakietowego.
    // Algorytm powinien priorytetowo zamówić Ser (dla Klienta 2), a jako drugą
    // opcję dobrać Mleko z progów minimalnych.
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], IngredientType::Cheese);
    EXPECT_EQ(result[1], IngredientType::Milk);
}

// ---------------------------------------------------------
// TEST 3: Weryfikacja mechanizmu wirtualnej konsumpcji zasobów
// ---------------------------------------------------------
TEST(DeliveryLogicTest, MultipleCustomersSameIngredient_DetectsVirtualShortage) {
    // Arrange: Sytuacja, w której popyt przewyższa bieżącą podaż na ten sam składnik
    std::vector<OrderRecord> queue = {
            { 101, IngredientType::Tomato },
            { 102, IngredientType::Tomato }
    };
    std::map<IngredientType, int> inventory = {
            { IngredientType::Tomato, 1 } // Dostępna fizycznie tylko 1 sztuka
    };
    std::map<IngredientType, int> thresholds = {
            { IngredientType::Flour, 3 }  // Próg pomocniczy dla drugiej paczki
    };

    // Act
    auto result = DeliveryLogic::CalculateWhatToOrder(queue, inventory, thresholds);

    // Assert: Weryfikacja spójności stanu dirty state.
    // Pierwszy klient wirtualnie konsumuje jedynego pomidora w pamięci algorytmu.
    // Algorytm musi wykryć brak dla drugiego klienta i wygenerować na niego zamówienie.
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], IngredientType::Tomato);
    EXPECT_EQ(result[1], IngredientType::Flour);
}

// ---------------------------------------------------------
// TEST 4: Generowanie paczek z samych braków strukturalnych
// ---------------------------------------------------------
TEST(DeliveryLogicTest, RefillsEmptyPantryWithTwoDistinctPackages) {
    // Arrange: Kolejka obsłużona, testujemy wyłącznie moduł kontroli zapasów
    std::vector<OrderRecord> queue = {
            { 101, IngredientType::Tomato }
    };

    std::map<IngredientType, int> inventory = {
            { IngredientType::Tomato, 1 }, // Zabezpiecza bieżące zamówienie
            { IngredientType::Milk, 1 },   // Poniżej progu
            { IngredientType::Egg, 0 }     // Poniżej progu
    };
    std::map<IngredientType, int> thresholds = {
            { IngredientType::Milk, 3 },
            { IngredientType::Egg, 2 }
    };

    // Act
    auto result = DeliveryLogic::CalculateWhatToOrder(queue, inventory, thresholds);

    // Assert: Zamówienia klientów są zabezpieczone, więc algorytm powinien
    // wygenerować 2 unikalne paczki na podstawie uszczuplonych zapasów strukturalnych.
    ASSERT_EQ(result.size(), 2);
    EXPECT_TRUE(std::find(result.begin(), result.end(), IngredientType::Milk) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(), IngredientType::Egg) != result.end());
    // Dodatkowy test na unikalność
    EXPECT_NE(result[0], result[1]);
}

// ---------------------------------------------------------
// TEST 5: Zmiana stanu domeny i wymuszenie unikalności elementów
// ---------------------------------------------------------
TEST(DeliveryLogicTest, EnsuresPackageUniquenessRegardlessOfDemand) {
    // Arrange: Symulacja ekstremalnego braku jednego składnika
    // dla wielu klientów ORAZ w progach minimalnych.
    std::vector<OrderRecord> queue = {
            { 102, IngredientType::Cheese },
            { 103, IngredientType::Cheese }
    };

    std::map<IngredientType, int> inventory = {
            { IngredientType::Cheese, 0 },
            { IngredientType::Ham, 1 }
    };

    std::map<IngredientType, int> thresholds = {
            { IngredientType::Cheese, 5 }, // Ser jest wymagany podwójnie
            { IngredientType::Ham, 3 }     // Szynka służy jako fallback
    };

    // Act
    auto result = DeliveryLogic::CalculateWhatToOrder(queue, inventory, thresholds);

    // Assert: Zabezpieczenie przed dublowaniem zawartości paczek.
    // Nawet jeśli Ser jest potrzebny zarówno do kolejki zamówień, jak i do progów,
    // algorytm wciąż musi wygenerować szynkę jako drugą, różną paczkę.
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], IngredientType::Cheese);
    EXPECT_EQ(result[1], IngredientType::Ham);
}