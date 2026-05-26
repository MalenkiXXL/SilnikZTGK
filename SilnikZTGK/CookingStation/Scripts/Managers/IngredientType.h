#pragma once
#include <cstdint>

enum class IngredientType : uint32_t
{
    None = 0,

    // --- SUROWE SKŁADNIKI ---
    Tomato,
    Cheese,
    Ham,
    Potato,
    Mushroom,
    Mozzarella,
    Flour,
    Milk,
    Egg,

    // --- POKROJONE SKŁADNIKI ---
    ChoppedTomato,
    ChoppedCheese,
    ChoppedHam,
    ChoppedPotato,    
    ChoppedMushroom,
    ChoppedMozzarella,

    // --- PRODUKTY POŚREDNIE ---
    RawDough,          // Ciasto wyjęte z miksera (Mąka + Mleko)
    RawKopytka,        // Surowe kopytka z miksera (Ziemniak + Mąka + Jajko)
    Baguette,          // Upieczona bagietka w całości
    CutBaguette,       // Bagietka po przekrojeniu na desce

    // --- GOTOWE DANIA (Do wydania) ---
    TomatoSoup,        // Zupa
    Sandwich,          // Kanapka
    Caprese,           // Caprese
    Kopytka            // Gotowe kopytka 
};