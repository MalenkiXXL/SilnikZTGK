#pragma once
#include <unordered_map>
#include <string>

class GameProgress {
public:
    // Mapa trzymaj¹ca odblokowane przepisy
    static inline std::unordered_map<std::string, bool> UnlockedRecipes;

    // Funkcja sprawdzaj¹ca czy przepis jest odkryty
    static bool IsRecipeUnlocked(const std::string& recipeName) {
        return UnlockedRecipes[recipeName];
    }

    // Funkcja odblokowuj¹ca przepis
    static void UnlockRecipe(const std::string& recipeName) {
        UnlockedRecipes[recipeName] = true;
    }
};