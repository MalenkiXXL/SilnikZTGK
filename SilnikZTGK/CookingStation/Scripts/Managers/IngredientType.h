#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <glm/gtc/quaternion.hpp>

enum class IngredientType : uint32_t
{
    None = 0,

    // Podstawowe składniki
    Tomato, ChoppedTomato, TomatoSoup, Caprese, Basil, Water,
    Cheese, ChoppedCheese, Mozzarella, ChoppedMozzarella,
    Ham, ChoppedHam,
    Milk, Flour, Potato,

    // Pieczywo i ciasta
    RawDough, Baguette, CutBaguette, Sandwich,
    Bread, Cupcake, Cookies,

    // Jajka
    Egg, EggWithoutShell, ChoppedEgg,
    FriedEgg, EggWithHam, Shakshuka,

    // Kanapki warianty
    TomatoCheeseSandwich, HamTomatoSandwich, HamCheeseSandwich, EggSandwich,

    // Makarony / Kopytka / Inne
    Kopytka, GoldenKopytka, Noodle, Spaghetti, Ramen, Fries,

    // Pizza
    RawPizzaDough, Pizza, MushroomPizza, BakedPizza, CheesePizza,
    SaucePizza, HamPizza, HamMushroomPizza, HamMushroomTomatoPizza,

    // Grzyby i warzywa
    Mushroom, ChoppedMushroom, Carrot,

    // Słodkie / Owoce
    Apple, Raspberry, Strawberry, ApplePie, Pancakes, Honey, MilkWithHoney, Candy,

    // Napoje
    Coffee, MilkCoffee, CoffeeBeans, ShakeCup, AppleShake, CoffeeShake, RaspberryShake, StrawberryShake
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
        case IngredientType::None:                      return "None";
        case IngredientType::Tomato:                    return "Tomato";
        case IngredientType::ChoppedTomato:             return "ChoppedTomato";
        case IngredientType::TomatoSoup:                return "TomatoSoup";
        case IngredientType::Caprese:                   return "Caprese";
        case IngredientType::Basil:                     return "Basil";
        case IngredientType::Water:                     return "Water";
        case IngredientType::Cheese:                    return "Cheese";
        case IngredientType::ChoppedCheese:             return "ChoppedCheese";
        case IngredientType::Ham:                       return "Ham";
        case IngredientType::ChoppedHam:                return "ChoppedHam";
        case IngredientType::Mozzarella:                return "Mozzarella";
        case IngredientType::ChoppedMozzarella:         return "ChoppedMozzarella";
        case IngredientType::Milk:                      return "Milk";
        case IngredientType::Flour:                     return "Flour";
        case IngredientType::Egg:                       return "Egg";
        case IngredientType::EggWithoutShell:           return "EggWithoutShell";
        case IngredientType::ChoppedEgg:                return "ChoppedEgg";
        case IngredientType::Potato:                    return "Potato";
        case IngredientType::RawDough:                  return "RawDough";
        case IngredientType::Baguette:                  return "Baguette";
        case IngredientType::CutBaguette:               return "CutBaguette";
        case IngredientType::Sandwich:                  return "Sandwich";
        case IngredientType::FriedEgg:                  return "FriedEgg";
        case IngredientType::EggWithHam:                return "EggWithHam";
        case IngredientType::Shakshuka:                 return "Shakshuka";
        case IngredientType::Bread:                     return "Bread";
        case IngredientType::Cupcake:                   return "Cupcake";
        case IngredientType::Cookies:                   return "Cookies";
        case IngredientType::TomatoCheeseSandwich:      return "TomatoCheeseSandwich";
        case IngredientType::HamTomatoSandwich:         return "HamTomatoSandwich";
        case IngredientType::HamCheeseSandwich:         return "HamCheeseSandwich";
        case IngredientType::EggSandwich:               return "EggSandwich";
        case IngredientType::Kopytka:                   return "Kopytka";
        case IngredientType::GoldenKopytka:             return "GoldenKopytka";
        case IngredientType::Noodle:                    return "Noodle";
        case IngredientType::Spaghetti:                 return "Spaghetti";
        case IngredientType::Ramen:                     return "Ramen";
        case IngredientType::Fries:                     return "Fries";
        case IngredientType::RawPizzaDough:             return "RawPizzaDough";
        case IngredientType::Pizza:                     return "Pizza";
        case IngredientType::MushroomPizza:             return "MushroomPizza";
        case IngredientType::BakedPizza:                return "BakedPizza";
        case IngredientType::CheesePizza:               return "CheesePizza";
        case IngredientType::SaucePizza:                return "SaucePizza";
        case IngredientType::HamPizza:                  return "HamPizza";
        case IngredientType::HamMushroomPizza:          return "HamMushroomPizza";
        case IngredientType::HamMushroomTomatoPizza:    return "HamMushroomTomatoPizza";
        case IngredientType::Mushroom:                  return "Mushroom";
        case IngredientType::ChoppedMushroom:           return "ChoppedMushroom";
        case IngredientType::Carrot:                    return "Carrot";
        case IngredientType::Apple:                     return "Apple";
        case IngredientType::Raspberry:                 return "Raspberry";
        case IngredientType::Strawberry:                return "Strawberry";
        case IngredientType::ApplePie:                  return "ApplePie";
        case IngredientType::Pancakes:                  return "Pancakes";
        case IngredientType::Honey:                     return "Honey";
        case IngredientType::MilkWithHoney:             return "MilkWithHoney";
        case IngredientType::Candy:                     return "Candy";
        case IngredientType::Coffee:                    return "Coffee";
        case IngredientType::MilkCoffee:                return "MilkCoffee";
        case IngredientType::CoffeeBeans:               return "CoffeeBeans";
        case IngredientType::ShakeCup:                  return "ShakeCup";
        case IngredientType::AppleShake:                return "AppleShake";
        case IngredientType::CoffeeShake:               return "CoffeeShake";
        case IngredientType::RaspberryShake:            return "RaspberryShake";
        case IngredientType::StrawberryShake:           return "StrawberryShake";
        default:                                        return "Unknown";
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
        case IngredientType::ChoppedHam:
            return { glm::vec3(7.5f), glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f) };
        case IngredientType::Mozzarella:
        case IngredientType::ChoppedMozzarella:
        case IngredientType::Caprese:
            return { glm::vec3(0.5f), glm::vec3(0.0f) };
        case IngredientType::Flour:
        case IngredientType::Baguette:
        case IngredientType::CutBaguette:
        case IngredientType::Sandwich:
            return { glm::vec3(6.0f), glm::vec3(0.0f) };
        case IngredientType::Milk:
            return { glm::vec3(0.4f), glm::vec3(0.0f, glm::radians(90.0f), 0.0f) };
        case IngredientType::Egg:
            return { glm::vec3(0.4f), glm::vec3(0.0f) };

        default:
            return { glm::vec3(1.0f), glm::vec3(0.0f) };
    }
}

inline std::string GetModelPath(IngredientType type)
{
    switch (type) {
        // Pomidory i pochodne
        case IngredientType::Tomato:                    return "assets://models/skladniki/pomidor/pomidor.gltf";
        case IngredientType::ChoppedTomato:             return "assets://models/skladniki/pomidor/pomidor-pokrojony.gltf";
        case IngredientType::TomatoSoup:                return "assets://models/skladniki/pomidor/pomidorowa.gltf";
        case IngredientType::Caprese:                   return "assets://models/skladniki/pomidor/caprese.gltf";
        case IngredientType::Basil:                     return "assets://models/skladniki/pomidor/bazylia.gltf";
        case IngredientType::Water:                     return "assets://models/skladniki/pomidor/woda.gltf";
        case IngredientType::Mozzarella:                return "assets://models/skladniki/pomidor/mozzarella.gltf";
        case IngredientType::ChoppedMozzarella:         return "assets://models/skladniki/pomidor/mozzarella-pokrojona.gltf";

            // Sery i Szynki
        case IngredientType::Cheese:                    return "assets://models/skladniki/ser/ser.gltf";
        case IngredientType::ChoppedCheese:             return "assets://models/skladniki/ser/ser-pokrojony.gltf";
        case IngredientType::Ham:                       return "assets://models/skladniki/szynka/szynka.gltf";
        case IngredientType::ChoppedHam:                return "assets://models/skladniki/szynka/szynka-pokrojona.gltf";

            // Pieczywo i ciasta
        case IngredientType::RawDough:                  return "assets://models/skladniki/ciasta_nieupieczone/ciasto.gltf";
        case IngredientType::Baguette:                  return "assets://models/skladniki/bagietka/bagietka.gltf";
        case IngredientType::CutBaguette:               return "assets://models/skladniki/bagietka/bagietka-przekrojona.gltf";
        case IngredientType::Sandwich:                  return "assets://models/skladniki/kanapki/kanapka.gltf";
        case IngredientType::Bread:                     return "assets://models/skladniki/chleb/chleb2.gltf";
        case IngredientType::Cupcake:                   return "assets://models/skladniki/babeczka/babeczka.gltf";
        case IngredientType::Cookies:                   return "assets://models/skladniki/ciastka/ciastka.gltf";
        case IngredientType::Flour:                     return "assets://models/skladniki/maka/maka.gltf";

            // Jajka
        case IngredientType::Egg:                       return "assets://models/skladniki/jajko_w_skorupce/egg_withshell.gltf";
        case IngredientType::EggWithoutShell:           return "assets://models/skladniki/jajko_bez/egg_withoutshell.gltf";
        case IngredientType::ChoppedEgg:                return "assets://models/skladniki/jajko_pokrojone/egg_cut.gltf";
        case IngredientType::FriedEgg:                  return "assets://models/skladniki/jajko-dania/jajo.gltf";
        case IngredientType::EggWithHam:                return "assets://models/skladniki/jajko-dania/jajo-bekon.gltf";
        case IngredientType::Shakshuka:                 return "assets://models/skladniki/jajko-dania/szakszuka.gltf";

            // Różne Kanapki
        case IngredientType::TomatoCheeseSandwich:      return "assets://models/skladniki/kanapki/kanapka_pomidor_ser.gltf";
        case IngredientType::HamTomatoSandwich:         return "assets://models/skladniki/kanapki/kanapka_szynka_pomidor.gltf";
        case IngredientType::HamCheeseSandwich:         return "assets://models/skladniki/kanapki/kanapka_szynka_ser.gltf";
        case IngredientType::EggSandwich:               return "assets://models/skladniki/kanapki/kanapka_z_jajkiem.gltf";

            // Obiadowe (Makarony, Ziemniaki, Frytki)
        case IngredientType::Potato:                    return "assets://models/skladniki/ziemniak/potato2.gltf";
        case IngredientType::Fries:                     return "assets://models/skladniki/frytki/frytki.gltf";
        case IngredientType::Kopytka:                   return "assets://models/skladniki/kopytka/kopytka.gltf";
        case IngredientType::GoldenKopytka:             return "assets://models/skladniki/kopytka/kopytka-zlote.gltf";
        case IngredientType::Noodle:                    return "assets://models/skladniki/makaron/noddle.gltf";
        case IngredientType::Spaghetti:                 return "assets://models/skladniki/spagetti/spagetti.gltf";
        case IngredientType::Ramen:                     return "assets://models/skladniki/ramen/ramen.gltf";

            // Pizza
        case IngredientType::RawPizzaDough:             return "assets://models/skladniki/ciasta_nieupieczone/ciasto-pizza.gltf";
        case IngredientType::Pizza:                     return "assets://models/skladniki/pizza/pizza.gltf";
        case IngredientType::MushroomPizza:             return "assets://models/skladniki/pizza/pizza_pieczarki.gltf";
        case IngredientType::BakedPizza:                return "assets://models/skladniki/pizza/pizza_upieczona.gltf";
        case IngredientType::CheesePizza:               return "assets://models/skladniki/pizza/pizza_z_serem.gltf";
        case IngredientType::SaucePizza:                return "assets://models/skladniki/pizza/pizza_z_sosem.gltf";
        case IngredientType::HamPizza:                  return "assets://models/skladniki/pizza/pizza_z_szynka.gltf";
        case IngredientType::HamMushroomPizza:          return "assets://models/skladniki/pizza/pizza_z_szynka_pieczarki.gltf";
        case IngredientType::HamMushroomTomatoPizza:    return "assets://models/skladniki/pizza/pizza_z_szynka_pieczarki_pomidor.gltf";

            // Warzywa i grzyby
        case IngredientType::Mushroom:                  return "assets://models/skladniki/pieczarka/pieczarka.gltf";
        case IngredientType::ChoppedMushroom:           return "assets://models/skladniki/pieczarka/pokrojona_pieczarka.gltf";
        case IngredientType::Carrot:                    return "assets://models/skladniki/marchewka/carrot1.gltf";

            // Owoce i desery
        case IngredientType::Apple:                     return "assets://models/skladniki/jablko/apple1.gltf";
        case IngredientType::ApplePie:                  return "assets://models/skladniki/szarlotka/apple_pie.gltf";
        case IngredientType::Raspberry:                 return "assets://models/skladniki/malina/malina.gltf";
        case IngredientType::Strawberry:                return "assets://models/skladniki/truskawka/strawberry.gltf";
        case IngredientType::Pancakes:                  return "assets://models/skladniki/nalesniki/pancakes.gltf";
        case IngredientType::Honey:                     return "assets://models/skladniki/miod/jar1.gltf";
        case IngredientType::Candy:                     return "assets://models/skladniki/cukierek/cukierek.gltf";

            // Napoje / Mleczne
        case IngredientType::Milk:                      return "assets://models/skladniki/mleko/milk.gltf";
        case IngredientType::MilkWithHoney:             return "assets://models/skladniki/mleko_z_miodem/milk_with_honey.gltf";
        case IngredientType::Coffee:                    return "assets://models/skladniki/napoje/kawa.gltf";
        case IngredientType::MilkCoffee:                return "assets://models/skladniki/napoje/kawa-mleko.gltf";
        case IngredientType::CoffeeBeans:               return "assets://models/skladniki/napoje/ziarnokawy.gltf";
        case IngredientType::ShakeCup:                  return "assets://models/skladniki/shake/kubek-shake.gltf";
        case IngredientType::AppleShake:                return "assets://models/skladniki/shake/shake-jablko.gltf";
        case IngredientType::CoffeeShake:               return "assets://models/skladniki/shake/shake-kawa.gltf";
        case IngredientType::RaspberryShake:            return "assets://models/skladniki/shake/shake-malina.gltf";
        case IngredientType::StrawberryShake:           return "assets://models/skladniki/shake/shake-truskawka.gltf";

        default: return "";
    }
}

inline std::string GetTagForIngredient(IngredientType type)
{
    return IngredientTypeToString(type);
}