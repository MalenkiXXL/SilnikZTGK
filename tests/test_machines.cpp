#include <gtest/gtest.h>

// Załadowanie niezbędnych nagłówków
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Scripts/Machines/MixerScript.h"
#include "CookingStation/Scripts/Machines/OvenScript.h"
#include "CookingStation/Scripts/Machines/PotScript.h"
#include "CookingStation/Scripts/Machines/CuttingBoardScript.h"

/**
 * @file test_machines.cpp
 * Zestaw testów jednostkowych dla stacji roboczych (Maszyn).
 * Weryfikuje logikę biznesową gry: walidację składników wejściowych,
 * limity pojemności maszyn oraz zabezpieczenia przed błędnymi akcjami gracza.
 */

// ---------------------------------------------------------
// TEST 1: Walidacja typów i pojemności w Mikserze
// ---------------------------------------------------------
// Sprawdza, czy mikser poprawnie przyjmuje dozwolone składniki
// oraz czy blokuje próbę przepełnienia stacji (maksymalnie 2 składniki).
TEST(MachineTests, MixerAcceptsOnlyFlourAndMilk) {
    MixerScript mixer;

    EXPECT_TRUE(mixer.AddIngredient(IngredientType::Flour)) << "Mikser powinien akceptowac make.";
    EXPECT_TRUE(mixer.AddIngredient(IngredientType::Milk)) << "Mikser powinien akceptowac mleko.";

    // Próba dodania trzeciego składnika do pełnej maszyny
    EXPECT_FALSE(mixer.AddIngredient(IngredientType::Flour)) << "Mikser powinien zablokowac dodanie trzeciego skladnika.";
}

// ---------------------------------------------------------
// TEST 2: Zabezpieczenie przed duplikatami w Mikserze
// ---------------------------------------------------------
// Weryfikuje, czy maszyna blokuje wrzucenie dwóch takich samych składników.
TEST(MachineTests, MixerRejectsDuplicateIngredients) {
    MixerScript mixer;

    EXPECT_TRUE(mixer.AddIngredient(IngredientType::Flour));

    // Próba wrzucenia mąki po raz drugi
    EXPECT_FALSE(mixer.AddIngredient(IngredientType::Flour)) << "Mikser nie powinien pozwolic na zduplikowanie skladnika.";
}

// ---------------------------------------------------------
// TEST 3: Wymagania wstępne składników (Piekarnik)
// ---------------------------------------------------------
// Upewnia się, że piekarnik odrzuca surowe substraty i przetwarza
// wyłącznie odpowiednio przygotowane obiekty (w tym przypadku wyrobione ciasto).
TEST(MachineTests, OvenAcceptsOnlyRawDough) {
    OvenScript oven;

    EXPECT_FALSE(oven.AddIngredient(IngredientType::Flour)) << "Piekarnik powinien odrzucic sama make.";
    EXPECT_FALSE(oven.AddIngredient(IngredientType::Tomato)) << "Piekarnik powinien odrzucic warzywa.";

    EXPECT_TRUE(oven.AddIngredient(IngredientType::RawDough)) << "Piekarnik powinien zaakceptowac wyrobione ciasto.";
    EXPECT_FALSE(oven.AddIngredient(IngredientType::RawDough)) << "Piekarnik nie powinien przyjac drugiego ciasta naraz.";
}

// ---------------------------------------------------------
// TEST 4: Walidacja stanu przetworzenia w Garnku
// ---------------------------------------------------------
// Testuje izolowaną logikę maszyny. Sprawdzane jest wyłącznie odrzucanie
// błędnych składników. (Udane dodanie wyzwala system cząsteczek i wizualiów 3D,
// które w wyizolowanym środowisku testowym słusznie nie są alokowane).
TEST(MachineTests, PotRequiresChoppedIngredients) {
    PotScript pot;

    // Surowy pomidor nie może trafić do zupy przed pokrojeniem
    EXPECT_FALSE(pot.AddIngredient(IngredientType::Tomato)) << "Garnek powinien odrzucic calego pomidora.";
}

// ---------------------------------------------------------
// TEST 5: Deska do krojenia filtruje niespójne obiekty
// ---------------------------------------------------------
TEST(MachineTests, CuttingBoardRejectsUncuttableItems) {
    CuttingBoardScript board;

    // Składniki płynne lub sypkie nie powinny móc zostać położone na desce
    EXPECT_FALSE(board.AddIngredient(IngredientType::Milk)) << "Deska nie powinna akceptowac mleka.";
    EXPECT_FALSE(board.AddIngredient(IngredientType::Flour)) << "Deska nie powinna akceptowac maki.";
}