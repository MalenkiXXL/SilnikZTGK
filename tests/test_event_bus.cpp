#include <gtest/gtest.h>

// Załadowanie pełnego kontekstu sceny, aby odtworzyć
// naturalne środowisko gry i uniknąć zależności kołowych plików nagłówkowych.
#include "CookingStation/Scene/Scene.h"

#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Events/EventBus.h"

/**
 * @file test_event_bus.cpp
 * Zestaw testów jednostkowych dla klasy EventBus.
 * Weryfikuje poprawne rejestrowanie, wyrejestrowywanie
 * oraz przesyłanie zdarzeń między systemami w silniku.
 */

class EventBusTest : public ::testing::Test {
protected:
    EventBus eventBus; // Nowa instancja tworzona dla każdego testu
};

// ---------------------------------------------------------
// TEST 1: Przekazywanie danych wewnątrz zdarzenia
// ---------------------------------------------------------
TEST_F(EventBusTest, SingleSubscriberReceivesEventWithCorrectData) {
    int receivedAmount = 0;
    bool eventFired = false;

    // Podpięcie pod zdarzenie zmiany pieniędzy
    eventBus.Subscribe<MoneyChangedEvent>([&](const MoneyChangedEvent &e) {
        eventFired = true;
        receivedAmount = e.NewAmount;
    });

    eventBus.Publish(MoneyChangedEvent{250});

    EXPECT_TRUE(eventFired) << "Callback nie zostal wywolany.";
    EXPECT_EQ(receivedAmount, 250) << "Przekazano bledne wartosci w zdarzeniu.";
}

// ---------------------------------------------------------
// TEST 2: Reakcja wielu subskrybentów na to samo zdarzenie
// ---------------------------------------------------------
TEST_F(EventBusTest, MultipleSubscribersReceiveSameEvent) {
    int callCount = 0;

    eventBus.Subscribe<UIReadyEvent>([&](const UIReadyEvent &) { callCount++; });
    eventBus.Subscribe<UIReadyEvent>([&](const UIReadyEvent &) { callCount++; });
    eventBus.Subscribe<UIReadyEvent>([&](const UIReadyEvent &) { callCount++; });

    eventBus.Publish(UIReadyEvent{});

    EXPECT_EQ(callCount, 3) << "Zdarzenie nie dotarlo do wszystkich subskrybentow.";
}

// ---------------------------------------------------------
// TEST 3: Poprawne działanie metody Unsubscribe
// ---------------------------------------------------------
TEST_F(EventBusTest, UnsubscribeRemovesListenerSuccessfully) {
    bool eventFired = false;

    auto subId = eventBus.Subscribe<GamePausedEvent>([&](const GamePausedEvent &) {
        eventFired = true;
    });

    // Odpinamy nasłuchiwanie przed wywołaniem eventu
    eventBus.Unsubscribe<GamePausedEvent>(subId);
    eventBus.Publish(GamePausedEvent{});

    EXPECT_FALSE(eventFired) << "Callback wywolany mimo odpiecia subskrypcji.";
}

// ---------------------------------------------------------
// TEST 4: Izolacja typów zdarzeń
// ---------------------------------------------------------
// Sprawdza, czy wysłanie jednego zdarzenia nie wywołuje nasłuchiwaczy innych typów
TEST_F(EventBusTest, DifferentEventsAreIsolated) {
    bool menuEventFired = false;
    bool gameEventFired = false;

    eventBus.Subscribe<ShowMainMenuEvent>([&](const ShowMainMenuEvent &) {
        menuEventFired = true;
    });
    eventBus.Subscribe<GameStartedEvent>([&](const GameStartedEvent &) {
        gameEventFired = true;
    });

    eventBus.Publish(GameStartedEvent{});

    EXPECT_TRUE(gameEventFired) << "Zdarzenie docelowe nie zostalo odebrane.";
    EXPECT_FALSE(menuEventFired) << "Wywolano callback dla blednego typu zdarzenia.";
}

// ---------------------------------------------------------
// TEST 5: Bezpieczeństwo publikacji bez subskrybentów
// ---------------------------------------------------------
TEST_F(EventBusTest, PublishWithNoSubscribersDoesNotThrow) {
    // Sprawdza, czy wywołanie Publish na pustym kontenerze nie powoduje błędów działania programu
    EXPECT_NO_THROW({
                        eventBus.Publish(DeliveryCollectedEvent{});
                    });
}