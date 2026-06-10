#include <gtest/gtest.h>

// Załadowanie modułów poddawanych integracji
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Scene/ecs.h"
#include "CookingStation/Events/EventBus.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Scripts/Delivery/DeliveryLogic.h"
#include "CookingStation/Scene/ScriptableEntity.h"


/**
 * @file test_integration.cpp
 * Zestaw testów integracyjnych weryfikujących poprawną komunikację
 * i współpracę pomiędzy kluczowymi systemami silnika (ECS, AI, EventBus).
 */

// ---------------------------------------------------------
// Struktury pomocnicze (Mocki) na potrzeby testów
// Pozwalają testować silnik bez zależności od konkretnych klas z gry
// ---------------------------------------------------------
struct MockStatsComponent {
    int stressLevel;
};

class MockScript : public ScriptableEntity {
public:
    int updateCount = 0;
    void OnUpdate(Timestep ts) override { updateCount++; }
};

// ---------------------------------------------------------
// Inicjalizacja środowiska testowego
// ---------------------------------------------------------
class IntegrationTest : public ::testing::Test {
protected:
    World world;

    void SetUp() override {
        // Rejestracja testowych komponentów w ECS
        world.RegisterComponent<MockStatsComponent>();
        world.RegisterComponent<NativeScriptComponent>();
    }
};

// ---------------------------------------------------------
// TEST INTEGRACYJNY 1: Komunikacja Systemu Zdarzeń z Pamięcią ECS
// ---------------------------------------------------------
// Sprawdza, czy zdarzenie wysłane przez EventBus potrafi poprawnie
// odszukać encję w pamięci ECS i bezpiecznie zmodyfikować jej stan.
TEST_F(IntegrationTest, EventBusSafelyModifiesECSState) {
    Entity player = world.BuildEntity().With<MockStatsComponent>({0}).Build();
    bool eventProcessed = false;

    // Subskrypcja zdarzenia połączona z manipulacją danymi ECS
    world.GetEventBus().Subscribe<MoneyChangedEvent>([&](const MoneyChangedEvent& e) {
        auto* stats = world.GetComponent<MockStatsComponent>(player);
        if (stats) {
            stats->stressLevel -= e.NewAmount; // Modyfikacja stanu na podstawie danych ze zdarzenia
            eventProcessed = true;
        }
    });

    // Publikacja zdarzenia symulującego zarobienie pieniędzy
    world.GetEventBus().Publish(MoneyChangedEvent{50});

    EXPECT_TRUE(eventProcessed) << "EventBus nie przekazal zdarzenia do funkcji nasluchujacej.";

    // Weryfikacja, czy zmiana faktycznie zapisała się w strukturach ECS
    auto* currentStats = world.GetComponent<MockStatsComponent>(player);
    ASSERT_NE(currentStats, nullptr);
    EXPECT_EQ(currentStats->stressLevel, -50) << "Stan komponentu w ECS nie ulegl zmianie.";
}

// ---------------------------------------------------------
// TEST INTEGRACYJNY 2: Zdarzenia Systemowe a Logika AI (Delivery)
// ---------------------------------------------------------
// Weryfikuje przepływ danych: czy zdarzenia generowane w grze potrafią
// zbudować poprawny zbiór danych wejściowych dla algorytmu decyzyjnego AI.
TEST_F(IntegrationTest, KitchenEventsTriggerDeliveryAILogic) {
    std::vector<OrderRecord> activeOrders;

    // Zbieranie informacji z logiki gry za pomocą zdarzeń
    world.GetEventBus().Subscribe<KitchenOrderPlacedEvent>([&](const KitchenOrderPlacedEvent& e) {
        activeOrders.push_back({e.Customer.id, e.WantedDish});
    });

    Entity customer1 = world.BuildEntity().Build();
    Entity customer2 = world.BuildEntity().Build();

    // Symulacja złożenia zamówień
    world.GetEventBus().Publish(KitchenOrderPlacedEvent{customer1, IngredientType::Tomato});
    world.GetEventBus().Publish(KitchenOrderPlacedEvent{customer2, IngredientType::Cheese});

    ASSERT_EQ(activeOrders.size(), 2) << "Zdarzenia nie wygenerowaly poprawnych rekordow zamowien.";

    // Przekazanie zebranych danych do podsystemu AI
    std::map<IngredientType, int> inventory = {{IngredientType::Tomato, 0}, {IngredientType::Cheese, 0}};
    std::map<IngredientType, int> thresholds = {};

    auto toOrder = DeliveryLogic::CalculateWhatToOrder(activeOrders, inventory, thresholds);

    // Sprawdzenie, czy algorytm podjął trafną decyzję na podstawie zdarzeń
    ASSERT_EQ(toOrder.size(), 2);
    EXPECT_EQ(toOrder[0], IngredientType::Tomato);
    EXPECT_EQ(toOrder[1], IngredientType::Cheese);
}

// ---------------------------------------------------------
// TEST INTEGRACYJNY 3: Pamięć ECS a wykonywanie polimorficznych skryptów
// ---------------------------------------------------------
// Sprawdza, czy przypisanie skryptu (C++) do komponentu ECS pozwala na
// poprawne instancjonowanie logiki, jej aktualizację klatka po klatce oraz bezpieczne usuwanie.
TEST_F(IntegrationTest, ECSBindsAndExecutesNativeScripts) {
    Entity machine = world.BuildEntity().With<NativeScriptComponent>({}).Build();

    auto* scriptComp = world.GetComponent<NativeScriptComponent>(machine);
    ASSERT_NE(scriptComp, nullptr);

    // Bindowanie klasy skryptu do encji
    scriptComp->AddScript<MockScript>("MockScript");

    ASSERT_EQ(scriptComp->Scripts.size(), 1);
    EXPECT_EQ(scriptComp->Scripts[0].Name, "MockScript");

    // Symulacja inicjalizacji sceny (stworzenie instancji skryptu)
    scriptComp->Scripts[0].Instance = scriptComp->Scripts[0].InstantiateScript();
    ASSERT_NE(scriptComp->Scripts[0].Instance, nullptr);

    // Symulacja pętli głównej gry (Game Loop)
    auto* mockInstance = static_cast<MockScript*>(scriptComp->Scripts[0].Instance);
    mockInstance->OnUpdate(Timestep(0.016f));

    EXPECT_EQ(mockInstance->updateCount, 1) << "Podpiety skrypt nie zostal prawidlowo zaktualizowany.";

    // Symulacja zniszczenia encji i czyszczenie pamięci
    scriptComp->Scripts[0].DestroyScript(&scriptComp->Scripts[0]);
    EXPECT_EQ(scriptComp->Scripts[0].Instance, nullptr) << "System nie zwolnil pamieci po zniszczeniu skryptu.";
}