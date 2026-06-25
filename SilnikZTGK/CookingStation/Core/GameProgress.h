#pragma once
#include <unordered_map>
#include <string>

class GameProgress {
public:
    static inline std::unordered_map<std::string, bool> UnlockedRecipes;

    static bool IsRecipeUnlocked(const std::string& recipeName) {
        return UnlockedRecipes[recipeName];
    }

    static void UnlockRecipe(const std::string& recipeName) {
        UnlockedRecipes[recipeName] = true;
    }

    static void Reset() {
        UnlockedRecipes.clear();
    }
};