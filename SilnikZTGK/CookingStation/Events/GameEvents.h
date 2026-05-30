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

// ---------------------------------------------------------------------------
// Zdarzenia nawigacji między warstwami (zamiast rzutowania po m_LayerStack)
// ---------------------------------------------------------------------------

// Publikuje GameGuiLayer → odbiera MainMenuLayer
// Sygnalizuje powrót do menu głównego po wyjściu z gry.
struct ShowMainMenuEvent {};

// Publikuje MainMenuLayer → odbiera GameGuiLayer
// Sygnalizuje, że scena gry została załadowana i GUI gry ma się pokazać.
struct GameStartedEvent {};