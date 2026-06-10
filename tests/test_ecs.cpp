#include <gtest/gtest.h>

// Załadowanie pełnego kontekstu sceny, by zapobiec błędom kompilacji i zależnościom
#include "CookingStation/Scene/Scene.h"

/**
 * @file test_ecs.cpp
 * Zestaw testów jednostkowych dla systemu ECS (Entity Component System).
 * Weryfikuje tworzenie i usuwanie encji, poprawność mechanizmu generacji,
 * zarządzanie pamięcią komponentów oraz operacje na kontenerach SparseSet.
 */

// ---------------------------------------------------------
// Struktury pomocnicze wykorzystywane wyłącznie w testach
// ---------------------------------------------------------
struct HealthComponent {
    int hp;
};

struct PositionComponent {
    float x, y;
};

// ---------------------------------------------------------
// Inicjalizacja środowiska przed każdym testem
// ---------------------------------------------------------
class ECSTest : public ::testing::Test {
protected:
    World world;

    void SetUp() override {
        world.RegisterComponent<HealthComponent>();
        world.RegisterComponent<PositionComponent>();
    }
};

// ---------------------------------------------------------
// TEST 1: Przydzielanie ID encji i aktualizacja generacji
// ---------------------------------------------------------
TEST_F(ECSTest, AllocatingAndFreeingEntitiesUpdatesGeneration) {
    Entity e1 = world.BuildEntity().Build();

    EXPECT_EQ(e1.id, 0);
    EXPECT_EQ(e1.generation, 0);

    world.DestroyEntity(e1);
    Entity e2 = world.BuildEntity().Build();

    // Weryfikacja recyklingu ID oraz podbicia generacji
    EXPECT_EQ(e2.id, 0) << "Nie odzyskano zwolnionego ID encji.";
    EXPECT_EQ(e2.generation, 1) << "Generacja nie zostala poprawnie zaktualizowana po usunieciu.";
}

// ---------------------------------------------------------
// TEST 2: Dodawanie i pobieranie przypisanych komponentów
// ---------------------------------------------------------
TEST_F(ECSTest, CanAddAndRetrieveComponents) {
    Entity player = world.BuildEntity()
            .With<HealthComponent>({100})
            .With<PositionComponent>({10.5f, 20.0f})
            .Build();

    HealthComponent* health = world.GetComponent<HealthComponent>(player);
    PositionComponent* pos = world.GetComponent<PositionComponent>(player);

    ASSERT_NE(health, nullptr) << "Brak przypisanego komponentu HealthComponent.";
    ASSERT_NE(pos, nullptr) << "Brak przypisanego komponentu PositionComponent.";
    EXPECT_EQ(health->hp, 100);
    EXPECT_FLOAT_EQ(pos->x, 10.5f);
}

// ---------------------------------------------------------
// TEST 3: Weryfikacja przestarzałych referencji (niezgodność generacji)
// ---------------------------------------------------------
// Sprawdza, czy system poprawnie odrzuca zapytania wykorzystujące
// referencję do usuniętego obiektu, którego ID zostało ponownie przydzielone.
TEST_F(ECSTest, GetComponentFailsIfEntityGenerationIsOld) {
    Entity oldPlayerReference = world.BuildEntity().With<HealthComponent>({50}).Build();

    world.DestroyEntity(oldPlayerReference);
    Entity newEnemy = world.BuildEntity().With<HealthComponent>({200}).Build();

    HealthComponent* oldHealth = world.GetComponent<HealthComponent>(oldPlayerReference);
    HealthComponent* newHealth = world.GetComponent<HealthComponent>(newEnemy);

    EXPECT_EQ(oldHealth, nullptr) << "Zwrocono komponent dla nieaktualnej generacji encji.";
    ASSERT_NE(newHealth, nullptr);
    EXPECT_EQ(newHealth->hp, 200);
}

// ---------------------------------------------------------
// TEST 4: Spójność pamięci w SparseSet po usunięciu elementu
// ---------------------------------------------------------
TEST_F(ECSTest, DestroyingEntityCleansUpComponentsAndMaintainsDenseArray) {
    Entity e0 = world.BuildEntity().With<HealthComponent>({10}).Build();
    Entity e1 = world.BuildEntity().With<HealthComponent>({20}).Build();
    Entity e2 = world.BuildEntity().With<HealthComponent>({30}).Build();

    // Usunięcie e1 wymusza relokację danych e2 na miejsce e1
    world.DestroyEntity(e1);

    EXPECT_EQ(world.GetComponent<HealthComponent>(e1), nullptr);

    // Weryfikacja, czy pozostałe komponenty nie zostały uszkodzone podczas relokacji
    ASSERT_NE(world.GetComponent<HealthComponent>(e0), nullptr);
    ASSERT_NE(world.GetComponent<HealthComponent>(e2), nullptr);
    EXPECT_EQ(world.GetComponent<HealthComponent>(e0)->hp, 10);
    EXPECT_EQ(world.GetComponent<HealthComponent>(e2)->hp, 30);
}

// ---------------------------------------------------------
// TEST 5: Bezpieczna obsługa niezarejestrowanych komponentów
// ---------------------------------------------------------
struct UnregisteredComponent { int dummy; };

TEST_F(ECSTest, AccessingUnregisteredComponentReturnsNullptrSafely) {
    Entity e = world.BuildEntity().Build();

    // Upewniamy się, że próba dostępu do błędnego komponentu nie przerywa działania programu
    EXPECT_NO_THROW({
                        UnregisteredComponent* comp = world.GetComponent<UnregisteredComponent>(e);
                        EXPECT_EQ(comp, nullptr);
                    });
}