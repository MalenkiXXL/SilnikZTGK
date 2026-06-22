#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scripts/Managers/IngredientType.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Core/AudioEngine.h"
#include <vector>
#include <algorithm>
#include <random>

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
        AudioEngine::Play("assets://sounds/put_ingredient_on_plate.mp3");

        SpawnIngredientVisual(type);

        CheckRecipes();

        GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                m_Entity, glm::vec3(0.2f, 1.0f, 0.2f), 1.5f, false
        });

        for (Entity e : m_VisualModels) {
            GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                    e, glm::vec3(0.2f, 1.0f, 0.2f), 1.5f, false
            });
        }

        return true;
    }

    bool ReceiveFinishedDish(Entity dishEntity)
    {
        if (m_CompletedDish != IngredientType::None || !m_Ingredients.empty())
        {
            spdlog::warn("Talerz: Odmawiam przyjecia dania. Mam juz kanapke lub skladniki!");
            return false;
        }

        auto* rel = GetScene()->GetWorld().GetComponent<RelationshipComponent>(m_Entity);
        if (rel) {
            std::size_t currentChildId = rel->FirstChild;
            while (currentChildId != std::numeric_limits<std::size_t>::max()) {
                auto* tag = GetScene()->GetWorld().GetComponentByID<TagComponent>(currentChildId);
                if (tag && tag->Tag == "UgotowaneDanie") {
                    spdlog::warn("Talerz: Odmawiam. Mam juz ugotowane danie z maszyny!");
                    return false;
                }
                auto* childRel = GetScene()->GetWorld().GetComponentByID<RelationshipComponent>(currentChildId);
                currentChildId = childRel ? childRel->NextSibling : std::numeric_limits<std::size_t>::max();
            }
        }


        auto* tagComp = GetScene()->GetWorld().GetComponent<TagComponent>(dishEntity);
        if (tagComp) tagComp->Tag = "UgotowaneDanie";

        auto* foodTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(dishEntity);
        if (foodTransform) {
            foodTransform->SetPosition(glm::vec3(0.0f, 0.15f, 0.0f));
        }

        GetScene()->SetParent(dishEntity, m_Entity);
        m_VisualModels.push_back(dishEntity);
        AudioEngine::Play("assets://sounds/plate_down.wav");

        spdlog::info("Talerz: Przyjeto gotowe danie z maszyny (rozpoznawane po Tagu)!");
        return true;
    }

private:

    void SpawnIngredientVisual(IngredientType type)
    {
        std::string modelPath = GetModelPath(type);
        if (modelPath.empty()) return;

        auto builder = GetScene()->GetWorld().BuildEntity();

        TransformComponent tc;
        int itemIndex = (int)m_Ingredients.size() - 1;

        float basePlateHeight = 0.08f;  // Wysokość dna Twojego głębokiego talerza
        float itemThickness = 0.04f;    // Grubość pojedynczego składnika

        float stackYOffset = basePlateHeight + (itemIndex * itemThickness);
        tc.SetPosition(glm::vec3(0.0f, stackYOffset, 0.0f));

        // SKALA
        IngredientMetadata meta = GetIngredientMetadata(type);
        tc.SetScale(meta.scale);

        // LOSOWA ROTACJA (Wokół osi Y)
        std::random_device rd;
        std::mt19937 gen(rd());

        std::uniform_real_distribution<float> distrib(-35.0f, 35.0f);
        float randomYRotation = distrib(gen);

        glm::vec3 finalRotation = meta.rotation;
        finalRotation.y += randomYRotation;

        tc.SetRotation(finalRotation);
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

        std::vector<IngredientType> capreseRecipe = {
            IngredientType::ChoppedTomato,
            IngredientType::ChoppedMozzarella
        };

        if (MatchesRecipe(capreseRecipe))
        {
            TransformIntoDish(IngredientType::Caprese);
        }

        if (MatchesRecipe(sandwichRecipe))
        {
            TransformIntoDish(IngredientType::Sandwich);
        }
    }

    void TransformIntoDish(IngredientType dishType)
    {
        spdlog::info("Talerz: Z�o�ono gotowe danie!");

        std::vector<IngredientType> historyIngredients = m_Ingredients;

        m_CompletedDish = dishType;
        m_Ingredients.clear();

        for (Entity e : m_VisualModels) {
            GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ e });
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
        mesh.ModelPtr = AssetManager::GetModel(GetModelPath(dishType));
        builder.With<MeshComponent>(mesh);

        builder.With<TagComponent>({ GetTagForIngredient(dishType) });

        Entity dishEntity = builder.Build();
        GetScene()->SetParent(dishEntity, m_Entity);
        m_VisualModels.push_back(dishEntity);

        DishHistory history;
        history.BaseIngredients = historyIngredients;
        history.OriginMachine = "Plate";
        GetScene()->GetWorld().GetEventBus().Publish(DishCreatedEvent{ dishEntity, history });

        GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                m_Entity, glm::vec3(1.0f, 0.8f, 0.0f), 2.0f, false
        });
        GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                dishEntity, glm::vec3(1.0f, 0.8f, 0.0f), 2.0f, false
        });
    }

};