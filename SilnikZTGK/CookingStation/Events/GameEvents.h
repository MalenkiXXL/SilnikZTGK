#pragma once
#include "CookingStation/Scripts/Managers/IngredientType.h"

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