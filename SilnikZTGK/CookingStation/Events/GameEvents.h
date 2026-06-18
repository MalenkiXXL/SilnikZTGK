#pragma once
#include "CookingStation/Scripts/Managers/IngredientType.h"
#include "CookingStation/Scene/Entity.h"
#include <cstddef>

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

struct ShowMainMenuEvent {};

struct GameStartedEvent {};

struct EntityHoveredEvent {
    Entity TargetEntity;
};



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


struct DishHistory {
    std::vector<IngredientType> BaseIngredients;
    std::string OriginMachine;
};

struct DishCreatedEvent {
    Entity FoodEntity;
    DishHistory History;
};

struct ValidateOrderRequestEvent {
    Entity Customer;
    Entity ServedFood;
    IngredientType WantedIngredient;
};

struct ValidateOrderResponseEvent {
    Entity Customer;
    bool IsCorrect;
};

struct KitchenOrderPlacedEvent {
    Entity Customer;
    IngredientType WantedDish;
};

struct CustomerServedEvent {
    Entity Customer;
    Entity ServedFood;
};

struct BuildModeToggledEvent {
    bool IsActive;
};

struct GamePausedEvent {};
struct GameResumedEvent {};

// Audio pause
struct PlayPauseSoundEvent {};
struct PlayUnpauseSoundEvent {};

//UI
struct TriggerHighlightEvent {
    Entity TargetEntity;
    glm::vec3 Color;
    float Duration = 0.6f;
    bool IsInfinite = false;
};

struct AudioSettingsChangedEvent {
    bool MusicEnabled;
    bool SoundsEnabled;
};