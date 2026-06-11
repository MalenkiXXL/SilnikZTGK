#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scripts/Managers/IngredientType.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include <vector>
#include <algorithm>

class PlateScript : public ScriptableEntity
{
public:
    std::vector<IngredientType> m_Ingredients;
    IngredientType m_CompletedDish = IngredientType::None;
    std::vector<Entity> m_VisualModels;

    bool AddIngredient(IngredientType type)
    {
        if (m_CompletedDish != IngredientType::None) return false;

        if (m_Ingredients.size() >= 5) return false;

        auto* tagSet = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        if (tagSet)
        {
            for (size_t i = 0; i < tagSet->dense.size(); ++i)
            {
                if (tagSet->dense[i].Tag == "UgotowaneDanie")
                {
                    Entity childEntity = tagSet->reverse[i];
                    if (GetScene()->GetParent(childEntity).id == m_Entity.id)
                    {
                        spdlog::warn("Talerz ma juz gotowe danie z maszyny!");
                        return false;
                    }
                }
            }
        }

        m_Ingredients.push_back(type);

        SpawnIngredientVisual(type);

        CheckRecipes();

        return true;
    }

private:
    std::string GetModelPath(IngredientType type)
    {
        switch (type) {
        case IngredientType::ChoppedTomato: return "assets://models/skladniki/pomidor/pomidor-pokrojony.gltf";
        case IngredientType::ChoppedCheese: return "assets://models/skladniki/ser/ser-pokrojony.gltf";
        case IngredientType::ChoppedHam:    return "assets://models/skladniki/szynka/szynka-pokrojona.gltf";
        case IngredientType::CutBaguette:   return "assets://models/skladniki/bagietka/bagietka-przekrojona.gltf";
        case IngredientType::RawDough:      return "assets://models/skladniki/maka/maka.gltf";
        case IngredientType::Baguette:      return "assets://models/skladniki/bagietka/bagietka.gltf";
        case IngredientType::Flour:         return "assets://models/skladniki/maka/maka.gltf";
        case IngredientType::Milk:          return "assets://models/skladniki/mleko/milk.gltf";
        default: return "";
        }
    }

    std::string GetTagForIngredient(IngredientType type)
    {
        switch (type) {
        case IngredientType::Tomato:
        case IngredientType::ChoppedTomato: return "Tomato";
        case IngredientType::Cheese:
        case IngredientType::ChoppedCheese: return "Cheese";
        case IngredientType::Ham:
        case IngredientType::ChoppedHam:    return "Ham";
        case IngredientType::Sandwich:      return "Sandwich";
        case IngredientType::CutBaguette:   return "CutBaguette";
        case IngredientType::RawDough:      return "RawDough";
        case IngredientType::Baguette:      return "Baguette";
        case IngredientType::Flour:         return "Flour";
        case IngredientType::Milk:          return "Milk";
        default: return "Unknown";
        }
    }

    void SpawnIngredientVisual(IngredientType type)
    {
        std::string modelPath = GetModelPath(type);
        if (modelPath.empty()) return;

        auto builder = GetScene()->GetWorld().BuildEntity();

        TransformComponent tc;
        float stackYOffset = 0.2f + (m_Ingredients.size() * 0.20f);
        tc.SetPosition(glm::vec3(0.0f, stackYOffset, 0.0f));

        IngredientMetadata meta = GetIngredientMetadata(type);
        tc.SetScale(meta.scale);
        tc.SetRotation(meta.rotation);
        builder.With<TransformComponent>(tc);

        MeshComponent mesh;
        mesh.ModelPtr = AssetManager::GetModel(modelPath);
        builder.With<MeshComponent>(mesh);

        builder.With<TagComponent>({ GetTagForIngredient(type) });

        Entity visualEntity = builder.Build();

        GetScene()->SetParent(visualEntity, m_Entity);
        m_VisualModels.push_back(visualEntity);
    }

    bool MatchesRecipe(const std::vector<IngredientType>& recipe)
    {
        if (m_Ingredients.size() != recipe.size()) return false;

        std::vector<IngredientType> myIng = m_Ingredients;
        std::vector<IngredientType> recIng = recipe;
        std::sort(myIng.begin(), myIng.end());
        std::sort(recIng.begin(), recIng.end());

        return myIng == recIng;
    }

    void CheckRecipes()
    {
        std::vector<IngredientType> sandwichRecipe = {
            IngredientType::CutBaguette,
            IngredientType::ChoppedHam,
            IngredientType::ChoppedCheese,
            IngredientType::ChoppedTomato
        };

        if (MatchesRecipe(sandwichRecipe))
        {
            TransformIntoDish(IngredientType::Sandwich, "assets://models/skladniki/bagietka/kanapka.gltf");
        }
    }

    void TransformIntoDish(IngredientType dishType, const std::string& dishModelPath)
    {
        spdlog::info("Talerz: Z�o�ono gotowe danie!");

        std::vector<IngredientType> historyIngredients = m_Ingredients;

        m_CompletedDish = dishType;
        m_Ingredients.clear();

        for (Entity e : m_VisualModels) {
            GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ e });
            e = { std::numeric_limits<std::size_t>::max(), 0 };
        }
        m_VisualModels.clear();

        auto builder = GetScene()->GetWorld().BuildEntity();

        TransformComponent tc;
        tc.SetPosition(glm::vec3(0.0f, 0.05f, 0.0f));

        IngredientMetadata meta = GetIngredientMetadata(dishType);
        tc.SetScale(meta.scale);
        tc.SetRotation(meta.rotation);
        builder.With<TransformComponent>(tc);

        MeshComponent mesh;
        mesh.ModelPtr = AssetManager::GetModel(dishModelPath);
        builder.With<MeshComponent>(mesh);

        builder.With<TagComponent>({ GetTagForIngredient(dishType) });

        Entity dishEntity = builder.Build();
        GetScene()->SetParent(dishEntity, m_Entity);
        m_VisualModels.push_back(dishEntity);

        DishHistory history;
        history.BaseIngredients = historyIngredients;
        history.OriginMachine = "Plate";
        GetScene()->GetWorld().GetEventBus().Publish(DishCreatedEvent{ dishEntity, history });
    }

private:
    Entity m_HighlightModelEntity = { std::numeric_limits<std::size_t>::max(), 0 };

public:
    void SetHighlight(bool isHighlighted)
    {
        const std::string targetShader = isHighlighted ? "HighlightShader" : "ModelShader";

        auto* mesh = GetComponent<MeshComponent>();
        if (mesh) mesh->ShaderName = targetShader;

        for (Entity e : m_VisualModels)
        {
            auto* childMesh = GetScene()->GetWorld().GetComponent<MeshComponent>(e);
            if (childMesh) childMesh->ShaderName = targetShader;
        }
    }
};