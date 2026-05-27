#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <glm/gtc/quaternion.hpp> 

enum class IngredientType : uint32_t
{
    None = 0,
    Tomato, ChoppedTomato,
    Cheese, ChoppedCheese,
    Ham, ChoppedHam,
    Mozzarella, ChoppedMozzarella,
    Milk, Flour, Egg, Potato,
    RawDough, Baguette, CutBaguette,
    Sandwich
    // ... 
};

// Struktura trzymająca metadane składnika
struct IngredientMetadata {
    glm::vec3 scale;
    glm::vec3 rotation;
};

// Metadane składnika, rotacja, skala
inline IngredientMetadata GetIngredientMetadata(IngredientType type)
{
    switch (type)
    {
    case IngredientType::Tomato:
        return { glm::vec3(0.8f)};
    case IngredientType::ChoppedTomato:
        return { glm::vec3(0.4f), glm::vec3(0.0f, 90.0f, 0.0f) };

    case IngredientType::Cheese:
        return { glm::vec3(7.5f), glm::vec3(0.0f, glm::radians(90.0f), 0.0f) };
    case IngredientType::ChoppedCheese:
        return { glm::vec3(7.5f), glm::vec3(glm::radians(90.0f), 0.0f, 0.0f) };

    case IngredientType::Ham:
        return { glm::vec3(7.5f), glm::vec3(glm::radians(90.0f), 0.0f, 0.0f) };
    case IngredientType::ChoppedHam:
        return { glm::vec3(7.5f), glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f) };

    case IngredientType::Flour:
        return { glm::vec3(6.0f), glm::vec3(0.0f) };

    case IngredientType::Milk:
        return { glm::vec3(0.4f), glm::vec3(0.0f, glm::radians(90.0f), 0.0f) };

    case IngredientType::Baguette:
        return { glm::vec3(6.0f), glm::vec3(0.0f, glm::radians(90.0f), 0.0f) };
    case IngredientType::CutBaguette:
        return { glm::vec3(6.0f), glm::vec3(0.0f) };

    case IngredientType::RawDough:
        return { glm::vec3(6.0f), glm::vec3(0.0f) }; 

    case IngredientType::Sandwich:
        return { glm::vec3(6.0f), glm::vec3(0.0f) };

    default:
        return { glm::vec3(1.0f), glm::vec3(0.0f) };
    }
}