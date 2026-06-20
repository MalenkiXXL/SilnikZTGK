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
    Sandwich, Caprese,
};

// Struktura trzymająca metadane składnika
struct IngredientMetadata {
    glm::vec3 scale;
    glm::vec3 rotation;
};

inline std::string IngredientTypeToString(IngredientType type)
{
    switch (type)
    {
        case IngredientType::None:              return "None";
        case IngredientType::Tomato:            return "Tomato";
        case IngredientType::ChoppedTomato:     return "ChoppedTomato";
        case IngredientType::Cheese:            return "Cheese";
        case IngredientType::ChoppedCheese:     return "ChoppedCheese";
        case IngredientType::Ham:               return "Ham";
        case IngredientType::ChoppedHam:        return "ChoppedHam";
        case IngredientType::Mozzarella:        return "Mozzarella";
        case IngredientType::ChoppedMozzarella: return "ChoppedMozzarella";
        case IngredientType::Milk:              return "Milk";
        case IngredientType::Flour:             return "Flour";
        case IngredientType::Egg:               return "Egg";
        case IngredientType::Potato:            return "Potato";
        case IngredientType::RawDough:          return "RawDough";
        case IngredientType::Baguette:          return "Baguette";
        case IngredientType::CutBaguette:       return "CutBaguette";
        case IngredientType::Sandwich:          return "Sandwich";
        case IngredientType::Caprese:           return "Caprese";
        default:                                return "Unknown";
    }
}

// Metadane składnika, rotacja, skala
inline IngredientMetadata GetIngredientMetadata(IngredientType type)
{
    switch (type)
    {
    case IngredientType::Tomato:
        return { glm::vec3(0.6f)};
    case IngredientType::ChoppedTomato:
        return { glm::vec3(0.4f), glm::vec3(0.0f, 90.0f, 0.0f) };

    case IngredientType::Cheese:
        return { glm::vec3(7.5f), glm::vec3(0.0f, glm::radians(90.0f), 0.0f) };
    case IngredientType::ChoppedCheese:
        return { glm::vec3(7.5f), glm::vec3(glm::radians(90.0f), 0.0f, 0.0f) };

    case IngredientType::Ham:
        return { glm::vec3(7.5f), glm::vec3(glm::radians(90.0f), 0.0f, 0.0f) };
    case IngredientType::Mozzarella:
        return { glm::vec3(0.5f), glm::vec3(0.0f) };
    case IngredientType::ChoppedMozzarella:
        return { glm::vec3(0.5f), glm::vec3(0.0f) };
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

    case IngredientType::Egg:
        return { glm::vec3(0.4f), glm::vec3(0.0f) };
    case IngredientType::Caprese:
        return { glm::vec3(0.5f), glm::vec3(0.0f) };
    default:
        return { glm::vec3(1.0f), glm::vec3(0.0f) };
    }
}