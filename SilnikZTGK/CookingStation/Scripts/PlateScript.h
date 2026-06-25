#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scripts/Managers/IngredientType.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h" 
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Core/GameProgress.h"
#include <vector>
#include <algorithm>
#include <random>

class PlateScript : public ScriptableEntity
{
public:
    std::vector<IngredientType> m_Ingredients;
    std::vector<IngredientType> m_DeepHistory; 
    std::vector<std::string> m_MachineHistory;
    IngredientType m_CompletedDish = IngredientType::None;
    std::vector<Entity> m_VisualModels;

    bool AddIngredient(IngredientType type, bool fromHelper = false)
    {
        if (m_CompletedDish != IngredientType::None)
        {
            if (!fromHelper) spdlog::warn("Talerz: Nie można dodawać składników do gotowego dania!");
            if (!fromHelper) AudioEngine::Play("assets://sounds/error.mp3");
            return false;
        }

        for (auto existing : m_Ingredients)
        {
            if (IsFinishedDish(existing))
            {
                if (!fromHelper) spdlog::warn("Talerz: Nie można dodawać składników do gotowego dania!");
                if (!fromHelper) AudioEngine::Play("assets://sounds/error.mp3");
                return false;
            }
        }

        if (!m_Ingredients.empty() && IsFinishedDish(type))
        {
            if (!fromHelper) spdlog::warn("Talerz: Nie można położyć gotowego dania na talerz z innymi składnikami!");
            if (!fromHelper) AudioEngine::Play("assets://sounds/error.mp3");
            return false;
        }

        if (!m_Ingredients.empty() && !m_VisualModels.empty() && IsFinishedDish(type))
        {
            if (!fromHelper) spdlog::warn("Talerz: Nie można położyć gotowego dania na talerz z innymi składnikami!");
            if (!fromHelper) AudioEngine::Play("assets://sounds/error.mp3");
            return false;
        }

        if (m_Ingredients.size() >= 5)
        {
            if (!fromHelper) spdlog::warn("Talerz: Osiągnięto limit składników (5)!");
            if (!fromHelper) AudioEngine::Play("assets://sounds/error.mp3");
            return false;
        }

        if (std::find(m_Ingredients.begin(), m_Ingredients.end(), type) != m_Ingredients.end())
        {
            if (!fromHelper) spdlog::warn("Talerz: Taki składnik ({}) już leży na talerzu!", IngredientTypeToString(type));
            if (!fromHelper) AudioEngine::Play("assets://sounds/error.mp3");
            return false;
        }

        if (IsRaw(type) && !m_Ingredients.empty())
        {
            if (!fromHelper) spdlog::warn("Talerz: Nie można klasc surowego ciasta na inne skladniki!");
            if (!fromHelper) AudioEngine::Play("assets://sounds/error.mp3");
            return false;
        }

        for (auto existing : m_Ingredients)
        {
            if (IsRaw(existing))
            {
                if (!fromHelper) spdlog::warn("Talerz: Nie można klasc niczego na surowe ciasto!");
                if (!fromHelper) AudioEngine::Play("assets://sounds/error.mp3");
                return false;
            }
        }

        bool hasBaguette = false;
        for (auto existing : m_Ingredients) {
            if (existing == IngredientType::CutBaguette) {
                hasBaguette = true;
                break;
            }
        }

        if (hasBaguette)
        {
            if (type != IngredientType::ChoppedCheese &&
                type != IngredientType::ChoppedHam &&
                type != IngredientType::ChoppedTomato)
            {
                if (!fromHelper) spdlog::warn("Talerz: Do kanapki mozesz dolozyc tylko pokrojony Ser, Szynke i Pomidora!");
                if (!fromHelper) AudioEngine::Play("assets://sounds/error.mp3");
                return false;
            }
        }

        if (type == IngredientType::CutBaguette && !m_Ingredients.empty())
        {
            for (auto existing : m_Ingredients)
            {
                if (existing != IngredientType::ChoppedCheese &&
                    existing != IngredientType::ChoppedHam &&
                    existing != IngredientType::ChoppedTomato)
                {
                    if (!fromHelper) spdlog::warn("Talerz: Nie mozna klasc pokrojonej bagietki na te skladniki!");
                    if (!fromHelper) AudioEngine::Play("assets://sounds/error.mp3");
                    return false;
                }
            }
        }

        bool incomingIsRaw = IsRaw(type);
        bool incomingIsChopped = IsChopped(type);

        for (auto existing : m_Ingredients)
        {
            if (incomingIsRaw && IsChopped(existing))
            {
                if (!fromHelper) spdlog::warn("Talerz: Zakaz kładzenia surowego na pokrojone!");
                if (!fromHelper) AudioEngine::Play("assets://sounds/error.mp3");
                return false;
            }
            if (incomingIsChopped && IsRaw(existing))
            {
                if (!fromHelper) spdlog::warn("Talerz: Zakaz kładzenia pokrojonego na surowe!");
                if (!fromHelper) AudioEngine::Play("assets://sounds/error.mp3");
                return false;
            }
        }

        if (!m_Ingredients.empty())
        {
            bool incomingIsSweet = IsSweet(type);
            bool incomingIsSavory = IsSavory(type);

            for (auto existing : m_Ingredients)
            {
                if (incomingIsSweet && IsSavory(existing))
                {
                    if (!fromHelper) spdlog::warn("Talerz: Nie można kłaść słodkiego na słone!");
                    if (!fromHelper) AudioEngine::Play("assets://sounds/error.mp3");
                    return false;
                }
                if (incomingIsSavory && IsSweet(existing))
                {
                    if (!fromHelper) spdlog::warn("Talerz: Nie można kłaść słonego na słodkie!");
                    if (!fromHelper) AudioEngine::Play("assets://sounds/error.mp3");
                    return false;
                }
            }
        }

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
                        if (!fromHelper) spdlog::warn("Talerz ma już gotowe danie z maszyny!");
                        if (!fromHelper) AudioEngine::Play("assets://sounds/error.mp3");
                        return false;
                    }
                }
            }
        }

        m_Ingredients.push_back(type);
        m_DeepHistory.push_back(type);
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

    bool ReceiveFinishedDish(Entity dishEntity, bool fromHelper = false)
    {
        if (m_CompletedDish != IngredientType::None || !m_Ingredients.empty())
        {
            if (!fromHelper) spdlog::warn("Talerz: Odmawiam przyjecia dania. Mam juz kanapke lub skladniki!");
            return false;
        }

        auto* rel = GetScene()->GetWorld().GetComponent<RelationshipComponent>(m_Entity);
        if (rel) {
            std::size_t currentChildId = rel->FirstChild;
            while (currentChildId != std::numeric_limits<std::size_t>::max()) {
                auto* tag = GetScene()->GetWorld().GetComponentByID<TagComponent>(currentChildId);
                if (tag && tag->Tag == "UgotowaneDanie") {
                    if (!fromHelper) spdlog::warn("Talerz: Odmawiam. Mam juz ugotowane danie z maszyny!");
                    return false;
                }
                auto* childRel = GetScene()->GetWorld().GetComponentByID<RelationshipComponent>(currentChildId);
                currentChildId = childRel ? childRel->NextSibling : std::numeric_limits<std::size_t>::max();
            }
        }

        if (GameManagerScript::s_Instance) {
            auto pastHistory = GameManagerScript::s_Instance->GetDishHistory(dishEntity.id);
            if (!pastHistory.empty()) m_DeepHistory.insert(m_DeepHistory.end(), pastHistory.begin(), pastHistory.end());
        }

        auto* tagComp = GetScene()->GetWorld().GetComponent<TagComponent>(dishEntity);
        if (tagComp)
        {
            if (tagComp->Tag == "BagietkaWPiekarniku") m_CompletedDish = IngredientType::Baguette;
            else if (tagComp->Tag == "SzarlotkaWPiekarniku") m_CompletedDish = IngredientType::ApplePie;
            else if (tagComp->Tag == "SpiacyChlebWPiekarniku") m_CompletedDish = IngredientType::SleepyBread;
            else if (tagComp->Tag == "BabeczkaWPiekarniku") m_CompletedDish = IngredientType::Cupcake;
        }

        if (m_CompletedDish == IngredientType::None)
        {
            auto* meshComp = GetScene()->GetWorld().GetComponent<MeshComponent>(dishEntity);
            if (meshComp && meshComp->ModelPtr)
            {
                for (uint32_t i = 1; i <= (uint32_t)IngredientType::RawKopytkaDough; ++i)
                {
                    IngredientType typeToCheck = (IngredientType)i;
                    std::string expectedPath = GetModelPath(typeToCheck);

                    if (!expectedPath.empty() && meshComp->ModelPtr == AssetManager::GetModel(expectedPath))
                    {
                        m_CompletedDish = typeToCheck;
                        spdlog::info("Talerz: Rozpoznano danie po modelu! To jest: {}", IngredientTypeToString(m_CompletedDish));
                        break;
                    }
                }
            }
        }

        if (tagComp) {
            tagComp->Tag = "UgotowaneDanie";
        }

        GetScene()->SetParent(dishEntity, m_Entity);

        auto* tc = GetScene()->GetWorld().GetComponent<TransformComponent>(dishEntity);
        if (tc)
        {
            if (m_CompletedDish != IngredientType::None)
            {
                IngredientMetadata meta = GetIngredientMetadata(m_CompletedDish);
                tc->SetScale(meta.scale);
                tc->SetRotation(meta.rotation);
                tc->SetPosition(glm::vec3(0.0f, 0.15f, 0.0f) + meta.offset);
            }
            else
            {
                tc->SetPosition(glm::vec3(0.0f, 0.15f, 0.0f));
                spdlog::warn("Talerz: Nie rozpoznano modelu ze slownika! Wymuszono centrowanie na X:0 Z:0.");
            }
        }

        m_VisualModels.push_back(dishEntity);
        AudioEngine::Play("assets://sounds/plate_down.wav");

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

        float basePlateHeight = 0.08f; 
        float itemThickness = 0.04f;   

        float stackYOffset = basePlateHeight + (itemIndex * itemThickness);
        tc.SetPosition(glm::vec3(0.0f, stackYOffset, 0.0f));

        // SKALA
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


    void CheckRecipes()
    {
        if (m_Ingredients.size() == 2)
        {
            bool hasTomato = false, hasMozzarella = false;

            for (auto ing : m_Ingredients) {
                if (ing == IngredientType::ChoppedTomato) hasTomato = true;
                if (ing == IngredientType::ChoppedMozzarella) hasMozzarella = true;
            }

            if (hasTomato && hasMozzarella) {
                TransformIntoDish(IngredientType::Caprese);
                return;
            }
        }
        if (m_Ingredients.size() == 4)
        {
            bool hasBread = false, hasHam = false, hasCheese = false, hasTomato = false;

            for (auto ing : m_Ingredients) {
                if (ing == IngredientType::CutBaguette) hasBread = true;
                if (ing == IngredientType::ChoppedHam) hasHam = true;
                if (ing == IngredientType::ChoppedCheese) hasCheese = true;
                if (ing == IngredientType::ChoppedTomato) hasTomato = true;
            }

            if (hasBread && hasHam && hasCheese && hasTomato) {
                TransformIntoDish(IngredientType::Sandwich);
                return;
            }
        }
    }

    void TransformIntoDish(IngredientType dishType)
    {
        spdlog::info("Talerz: Złożono gotowe danie!");

        if (dishType == IngredientType::Sandwich && !GameProgress::IsRecipeUnlocked("Sandwich"))
        {
            GameProgress::UnlockRecipe("Sandwich");
            spdlog::info("Książka Kucharska: Przepis na Sandwich odblokowany!");
        }
        else if (dishType == IngredientType::Caprese && !GameProgress::IsRecipeUnlocked("Caprese"))
        {
            GameProgress::UnlockRecipe("Caprese"); 
            spdlog::info("Książka Kucharska: Przepis na Caprese odblokowany!");
        }

        std::vector<IngredientType> historyIngredients = m_DeepHistory; 
        std::vector<std::string> historyMachines = m_MachineHistory;
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
        history.MachineHistory = historyMachines;
        history.OriginMachine = "Plate";
        GetScene()->GetWorld().GetEventBus().Publish(DishCreatedEvent{ dishEntity, history });

        GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                m_Entity, glm::vec3(1.0f, 0.8f, 0.0f), 2.0f, false
            });
        GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                dishEntity, glm::vec3(1.0f, 0.8f, 0.0f), 2.0f, false
            });

        m_DeepHistory.clear();
        m_MachineHistory.clear();
    }

};