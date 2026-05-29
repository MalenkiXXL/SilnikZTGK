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
        default: return "";
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

        Entity visualEntity = builder.Build();

        // Poniewa¿ robimy SetParent, nasza pozycja (0.0, offset, 0.0) na³o¿y siê idealnie na œrodek talerza
        GetScene()->SetParent(visualEntity, m_Entity);
        m_VisualModels.push_back(visualEntity);
    }

    bool MatchesRecipe(const std::vector<IngredientType>& recipe)
    {
        if (m_Ingredients.size() != recipe.size()) return false;

        // Kopiujemy i sortujemy, ¿eby kolejnoœæ wrzucania nie mia³a znaczenia
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
        spdlog::info("Talerz: Z³o¿ono gotowe danie!");
        m_CompletedDish = dishType;
        m_Ingredients.clear();

        for (Entity e : m_VisualModels) {
            auto* tf = GetScene()->GetWorld().GetComponent<TransformComponent>(e);
            if (tf) tf->SetPosition(glm::vec3(0.0f, -1000.0f, 0.0f));
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

        Entity dishEntity = builder.Build();
        GetScene()->SetParent(dishEntity, m_Entity);
        m_VisualModels.push_back(dishEntity);
    }
};