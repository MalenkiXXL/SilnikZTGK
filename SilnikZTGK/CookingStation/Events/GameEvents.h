#pragma once
#include "CookingStation/Scripts/Managers/IngredientType.h"
#include "CookingStation/Scene/Entity.h"
#include <cstddef>

// ---------------------------------------------------------------------------
// Zdarzenia rozgrywki
// ---------------------------------------------------------------------------

struct IngredientUsedEvent {
    IngredientType Type;
    int Amount;
};

struct UIReadyEvent {};

struct MoneyChangedEvent {
    int NewAmount;
};

struct InventoryChangedEvent {
    IngredientType Type;
    int NewAmount;
};

struct CollisionEvent {
    Entity EntityA;
    Entity EntityB;
};

struct EntityClickedEvent {
    Entity TargetEntity;
    int MouseButton;
};

struct AddIngredientEvent {
    IngredientType Type;
    int Amount;
};

struct MachinePickedUpEvent {
    Entity TargetMachine;
};

struct OrderFulfilledEvent {
    float RewardAmount;
    OrderFulfilledEvent(float reward) : RewardAmount(reward) {}
};

struct EntityDestroyRequestEvent {
    Entity TargetEntity;
};

struct EntityDestroyedEvent {
    Entity TargetEntity;
};

struct StartDragRequestEvent {
    IngredientType Type;
    std::string ModelPath;
};

// Delivery -------

struct DeliveryCollectedEvent {};

struct CarArrivedEvent
{
    glm::vec3 DropPosition;
};

struct ConfigurePackageEvent
{
    Entity TargetEntity;
    IngredientType Type;
    int Amount;
};

struct PackageSpawnedEvent
{
    Entity TargetEntity;
};
// ----------------


// ---------------------------------------------------------------------------
// Zdarzenia nawigacji między warstwami (zamiast rzutowania po m_LayerStack)
// ---------------------------------------------------------------------------

// Publikuje GameGuiLayer → odbiera MainMenuLayer
// Sygnalizuje powrót do menu głównego po wyjściu z gry.
struct ShowMainMenuEvent {};

// Publikuje MainMenuLayer → odbiera GameGuiLayer
// Sygnalizuje, że scena gry została załadowana i GUI gry ma się pokazać.
struct GameStartedEvent {};

struct EntityHoveredEvent {
    Entity TargetEntity;
};




// DO SKRYPTOW KELNERA

struct CustomerSeatedEvent {
    Entity Customer;
};

struct PlateReadyEvent {
    Entity Plate;
};

struct OrderTakenEvent {
    Entity Customer;
};

struct PlateGrabbedEvent {
    Entity Plate;
};


// Struktura przechowująca DNA naszej potrawy
struct DishHistory {
    std::vector<IngredientType> BaseIngredients;
    std::string OriginMachine;
};

// Event wysyłany, gdy nowa potrawa (lub przetworzony składnik) pojawia się w świecie
struct DishCreatedEvent {
    Entity FoodEntity;
    DishHistory History;
};

// Event z zapytaniem od klienta "Czy to co dostałem, to to, czego chciałem?"
struct ValidateOrderRequestEvent {
    Entity Customer;
    Entity ServedFood;
    IngredientType WantedIngredient;
};

// Odpowiedź od systemu dla konkretnego klienta
struct ValidateOrderResponseEvent {
    Entity Customer;
    bool IsCorrect;
};

// Event wysyłany przez klienta do magazynu, gdy kelner zbierze zamówienie
struct KitchenOrderPlacedEvent {
    Entity Customer;
    IngredientType WantedDish;
};

// Zmodyfikowany CustomerServedEvent - teraz przekazujemy encję jedzenia, a nie zbiór tagów
struct CustomerServedEvent {
    Entity Customer;
    Entity ServedFood;
};

struct GamePausedEvent {};
struct GameResumedEvent {};