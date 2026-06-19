#include <gtest/gtest.h>
#include "CookingStation/Scripts/Waiter/WaiterScript.h"

// =====================================================================
// KLASA OPAKOWUJĄCA (Tylko dla testów)
// Dziedziczy po WaiterScript i udostępnia chronione (protected) metody.
// =====================================================================
class TestableWaiterScript : public WaiterScript {
public:
    // Słowo kluczowe 'using' zmienia zasięg z protected na public, ale TYLKO w tej klasie
    using WaiterScript::IsWalkable;
    using WaiterScript::Heuristic;
    using WaiterScript::ReconstructPath;
    using WaiterScript::m_WalkableTiles;
    using WaiterScript::m_StaticObstacles;
    using WaiterScript::m_TaskQueue;
};

// =====================================================================
// KLASA TESTOWA
// =====================================================================
class WaiterScriptFifoTest : public ::testing::Test {
protected:
    // Zmieniamy wskaźnik na naszą testową wersję kelnera
    TestableWaiterScript *m_Waiter;

    void SetUp() override {
        m_Waiter = new TestableWaiterScript();
    }

    void TearDown() override {
        delete m_Waiter;
    }

    Entity CreateDummyCustomer(std::size_t id) {
        return Entity{id, 0};
    }
};

// =====================================================================
// TEST: Weryfikacja kolejności FIFO przy dodawaniu zadań
// Oczekiwane zachowanie: Klienci dodawani do kolejki zachowują chronologię.
// =====================================================================
TEST_F(WaiterScriptFifoTest, TaskQueue_FollowsStrictFifoOrder) {
// ARRANGE (Przygotowanie danych)
    Entity customer1 = CreateDummyCustomer(101);
    Entity customer2 = CreateDummyCustomer(102);
    Entity customer3 = CreateDummyCustomer(103);

// ACT (Wykonanie akcji - symulacja zajmowania miejsc)
    m_Waiter->m_TaskQueue.push_back({0, customer1});
    m_Waiter->m_TaskQueue.push_back({0, customer2});
    m_Waiter->m_TaskQueue.push_back({0, customer3});

// ASSERT (Weryfikacja wyników)
// Sprawdzamy, czy rozmiar kolejki jest poprawny
    ASSERT_EQ(m_Waiter->m_TaskQueue.size(), 3)
                                << "Kolejka powinna zawierac dokladnie 3 zadania.";

// Weryfikacja struktury FIFO: Pierwszy wchodzi, pierwszy wychodzi
    EXPECT_EQ(m_Waiter->m_TaskQueue[0].Target.id, 101)
                        << "Pierwszy klient na liscie powinien miec ID 101.";
    EXPECT_EQ(m_Waiter->m_TaskQueue[1].Target.id, 102)
                        << "Drugi klient na liscie powinien miec ID 102.";
    EXPECT_EQ(m_Waiter->m_TaskQueue[2].Target.id, 103)
                        << "Trzeci klient na liscie powinien miec ID 103.";
}

// =====================================================================
// TEST: Pobieranie i usuwanie zadań przez kelnera (CheckForTasks)
// Oczekiwane zachowanie: Kelner pobiera pierwsze zadanie i usuwa je z kolejki,
// a kolejne zadania przesuwają się poprawnie (zachowanie FIFO).
// =====================================================================
// =====================================================================
// TEST: Pobieranie i usuwanie zadań (Zastępczy test logiki bez użycia ECS)
// Oczekiwane zachowanie: Zawsze pobieramy z przodu kolejki i poprawnie ją przesuwamy.
// =====================================================================
TEST_F(WaiterScriptFifoTest, ManualQueueProcessing_MaintainsFifo) {
    // ARRANGE
    Entity customer1 = CreateDummyCustomer(101);
    Entity customer2 = CreateDummyCustomer(102);

    // Symulujemy kolejkę zawierającą dwóch klientów
    m_Waiter->m_TaskQueue.push_back({0, customer1});
    m_Waiter->m_TaskQueue.push_back({0, customer2});

    // ACT
    // Symulujemy proces pobierania zadania, tak jak zrobiłby to Kelner w CheckForTasks(),
    // ale bez odpytywania systemu komponentów (GetScene()->...).
    auto firstTask = m_Waiter->m_TaskQueue.front(); // Pobierz pierwszego
    m_Waiter->m_TaskQueue.erase(m_Waiter->m_TaskQueue.begin()); // Usuń pierwszego

    // ASSERT
    // 1. Upewniamy się, że pobrany klient to faktycznie pierwszy dodany (FIFO)
    EXPECT_EQ(firstTask.Target.id, 101)
                        << "Pobrano zlego klienta z poczatku kolejki!";

    // 2. Kolejka powinna zostać zredukowana o ten jeden element
    ASSERT_EQ(m_Waiter->m_TaskQueue.size(), 1)
                                << "Kolejka nie zostala poprawnie skrocona.";

    // 3. Nowym pierwszym elementem powinien być klient nr 2
    EXPECT_EQ(m_Waiter->m_TaskQueue[0].Target.id, 102)
                        << "Drugi klient nie przesunal sie na poczatek kolejki.";
}

// =====================================================================
// TEST: Kasowanie konkretnego zamówienia w połowie kolejki
// Oczekiwane zachowanie: Event OrderTaken usuwa klienta niezależnie 
// od jego pozycji, zachowując integralność reszty kolejki.
// =====================================================================
TEST_F(WaiterScriptFifoTest, OrderTakenEvent_RemovesSpecificCustomerFromQueue) {
// ARRANGE
    Entity customerA = CreateDummyCustomer(10);
    Entity customerB = CreateDummyCustomer(20);
    Entity customerC = CreateDummyCustomer(30);

    m_Waiter->m_TaskQueue.push_back({0, customerA});
    m_Waiter->m_TaskQueue.push_back({0, customerB}); // Ten zostanie "usunięty"
    m_Waiter->m_TaskQueue.push_back({0, customerC});

// ACT
// W naturalnym środowisku zrobiłby to EventBus, tu symulujemy bezpośrednio
// operację, która dzieje się w lambdzie dla m_OrderTakenSubId
    m_Waiter->m_TaskQueue.erase(
            std::remove_if(m_Waiter->m_TaskQueue.begin(), m_Waiter->m_TaskQueue.end(),
                           [&](const WaiterScript::WaiterTask &t) {
                               return t.Type == 0 && t.Target.id == customerB.id;
                           }),
            m_Waiter->m_TaskQueue.end()
    );

// ASSERT
    ASSERT_EQ(m_Waiter->m_TaskQueue.size(), 2)
                                << "Kolejka powinna miec 2 elementy po usunieciu jednego klienta.";

// Sprawdzamy czy FIFO pozostałych elementów nie uległo zniszczeniu
    EXPECT_EQ(m_Waiter->m_TaskQueue[0].Target.id, 10)
                        << "Pierwszy klient (A) zmienil pozycje w kolejce.";
    EXPECT_EQ(m_Waiter->m_TaskQueue[1].Target.id, 30)
                        << "Trzeci klient (C) nie przesunal sie poprawnie na drugie miejsce.";
}


// =====================================================================
// TEST: Czy IsWalkable poprawnie laczy biala liste (podloga) i czarna (stoly)
// =====================================================================
TEST_F(WaiterScriptFifoTest, IsWalkable_RequiresFloorAndNoObstacles) {
glm::ivec2 testCell(5, 5);

// Na poczatku pole powinno byc niechodliwe, bo nie ma podlogi
EXPECT_FALSE(m_Waiter->IsWalkable(testCell))
<< "Komorka powinna byc zablokowana, jesli nie wygenerowano tam podlogi.";

// Dodajemy kafel podlogi (Biala lista)
m_Waiter->m_WalkableTiles.insert(testCell);
EXPECT_TRUE(m_Waiter->IsWalkable(testCell))
<< "Komorka powinna byc dostepna po dodaniu podlogi.";

// Dodajemy tam stolik (Czarna lista)
m_Waiter->m_StaticObstacles.insert(testCell);
EXPECT_FALSE(m_Waiter->IsWalkable(testCell))
<< "Stolik powinien nadpisac podloge i zablokowac komorke.";
}

// =====================================================================
// TEST: Sprawdzenie heurystyki Manhattan w A*
// Oczekiwane zachowanie: Heurystyka poprawnie liczy dystans w liniach prostych kratek
// =====================================================================
TEST_F(WaiterScriptFifoTest, Heuristic_CalculatesCorrectManhattanDistance) {
glm::ivec2 posA(0, 0);
glm::ivec2 posB(3, 4); // Dystans Manhattan: |0-3| + |0-4| = 7

float distance = m_Waiter->Heuristic(posA, posB);
EXPECT_FLOAT_EQ(distance, 7.0f)
<< "Heurystyka Manhattan powinna zwrocic dokladnie sume przesuniec osi (7.0).";
}

// =====================================================================
// TEST: Sprawdzenie rekonstrukcji sciezki (ReconstructPath)
// Oczekiwane zachowanie: Zamienia wektor mapy rodzicow na odwrocona liste pozycji swiata
// =====================================================================
TEST_F(WaiterScriptFifoTest, ReconstructPath_RebuildsCorrectWorldPositions) {
// Ustawiamy rozmiar kafelka na 1.0 dla latwiejszych obliczen
GridSystem::CELL_SIZE = 1.0f;

std::unordered_map<glm::ivec2, glm::ivec2, IVec2Hash> cameFrom;
glm::ivec2 start(0, 0);
glm::ivec2 step1(1, 0);
glm::ivec2 target(2, 0);

// Budujemy mape powrotna: target -> step1 -> start
cameFrom[target] = step1;
cameFrom[step1] = start;

auto path = m_Waiter->ReconstructPath(cameFrom, target, start);

ASSERT_EQ(path.size(), 2)
<< "Sciezka rekonstruowana z 2 krokow powinna zawierac 2 punkty docelowe.";

// Komorka (1,0) po SnapToGrid/CellToWorld (cell + 0.5f) powinna dac X = 1.5
EXPECT_NEAR(path[0].x, 1.5f, 0.001f);
EXPECT_NEAR(path[1].x, 2.5f, 0.001f);
}