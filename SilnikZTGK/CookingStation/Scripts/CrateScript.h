#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scripts/Managers/IngredientType.h"
#include "CookingStation/Scripts/Plates/ItemScript.h" 
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include "CookingStation/Events/GameEvents.h"
#include <glm/glm.hpp>
#include <limits>
#include <string>

class CrateScript : public ScriptableEntity
{
public:
    IngredientType m_CrateIngredient = IngredientType::None;

    Entity m_VisualFood = { std::numeric_limits<std::size_t>::max(), 0 };
    float m_SpawnCooldown = 0.0f;
    bool m_HasStock = false;
    bool m_IsInitialized = false;
    bool m_WasHovered = false;

    void OnCreate() override
    {
        auto* tagComp = GetComponent<TagComponent>();
        if (tagComp) {
            std::string name = tagComp->Tag;
            if (name.find("Tomato") != std::string::npos || name.find("Pomidor") != std::string::npos)
                m_CrateIngredient = IngredientType::Tomato;
            else if (name.find("Cheese") != std::string::npos || name.find("Ser") != std::string::npos)
                m_CrateIngredient = IngredientType::Cheese;
            else if (name.find("Ham") != std::string::npos || name.find("Szynka") != std::string::npos)
                m_CrateIngredient = IngredientType::Ham;
            else if (name.find("Baguette") != std::string::npos || name.find("Bagietka") != std::string::npos)
                m_CrateIngredient = IngredientType::Baguette;
            else if (name.find("Milk") != std::string::npos || name.find("Mleko") != std::string::npos)
                m_CrateIngredient = IngredientType::Milk;
            else if (name.find("Flour") != std::string::npos || name.find("Maka") != std::string::npos)
                m_CrateIngredient = IngredientType::Flour;
            else if (name.find("Egg") != std::string::npos || name.find("Jajko") != std::string::npos)
                m_CrateIngredient = IngredientType::Egg;
            else if (name.find("Mozzarella") != std::string::npos || name.find("Mozzarela") != std::string::npos)
                m_CrateIngredient = IngredientType::Mozzarella;
            else if (name.find("Apple") != std::string::npos || name.find("Jablko") != std::string::npos)
                m_CrateIngredient = IngredientType::Apple;
            else if (name.find("Raspberry") != std::string::npos || name.find("Malina") != std::string::npos)
                m_CrateIngredient = IngredientType::Raspberry;
            else if (name.find("Strawberry") != std::string::npos || name.find("Truskawka") != std::string::npos)
                m_CrateIngredient = IngredientType::Strawberry;
            else if (name.find("CoffeeBeans") != std::string::npos || name.find("Kawa") != std::string::npos || name.find("Ziarna") != std::string::npos)
                m_CrateIngredient = IngredientType::CoffeeBeans;
            else if (name.find("SleepyDust") != std::string::npos || name.find("Pyl") != std::string::npos)
                m_CrateIngredient = IngredientType::SleepyDust;
            else if (name.find("Yawn") != std::string::npos || name.find("Ziewniecie") != std::string::npos)
                m_CrateIngredient = IngredientType::Yawn;
            else if (name.find("Potato") != std::string::npos || name.find("Ziemniak") != std::string::npos)
                m_CrateIngredient = IngredientType::Potato;
        }

        if (m_CrateIngredient == IngredientType::None) {
            spdlog::error("Skrzynka o ID {} ma nierozpoznany tag! Jest pusta i nie bedzie dzialac.", m_Entity.id);
        }

        m_ClickSubId = GetScene()->GetWorld().GetEventBus().Subscribe<EntityClickedEvent>(
            [this](const EntityClickedEvent& e) {
                if (e.TargetEntity.id == m_Entity.id) {
                    this->HandleClick();
                }
            }
        );
    }

    void OnDestroy() override
    {
        auto* scene = GetScene();
        if (scene) {
            if (m_ClickSubId != 0) {
                scene->GetWorld().GetEventBus().Unsubscribe<EntityClickedEvent>(m_ClickSubId);
            }
            if (m_VisualFood.id != std::numeric_limits<std::size_t>::max()) {
                scene->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_VisualFood });
            }
        }
    }

    bool isMIsHovered() const {
        return m_IsHovered;
    }


    void HandleClick()
    {
        if (m_CrateIngredient == IngredientType::None) return;
        if (m_SpawnCooldown > 0.0f || Input::IsUICapturingMouse() || MachineScript::GlobalIsMachineHeld) return;

        if (m_HasStock)
        {
            if (SpawnIngredientOnConveyor()) {
                m_SpawnCooldown = 0.2f;

                AudioEngine::Play("CookingStation/Assets/sounds/put_on_conveyor.mp3");

                GetScene()->GetWorld().GetEventBus().Publish(IngredientUsedEvent{ m_CrateIngredient, 1 });
            }
            else {

                AudioEngine::Play("CookingStation/Assets/sounds/error.mp3");
                TriggerErrorHighlight();
            }
        }
        else
        {
            spdlog::warn("Skrzynka: Brak zapasow tego skladnika w magazynie (0 sztuk)!");

            AudioEngine::Play("CookingStation/Assets/sounds/error.mp3");
            TriggerErrorHighlight();
        }
    }

    void OnUpdate(Timestep ts) override
    {
        if (m_CrateIngredient == IngredientType::None) return;

        if (m_SpawnCooldown > 0.0f) {
            m_SpawnCooldown -= ts.GetSeconds();
        }

        int currentStock = GameManagerScript::s_Instance ? GameManagerScript::s_Instance->GetIngredientCount(m_CrateIngredient) : 0;

        bool shouldHaveStock = (currentStock > 0);

        if (!m_IsInitialized || shouldHaveStock != m_HasStock) {
            m_HasStock = shouldHaveStock;
            m_IsInitialized = true;
            UpdateVisuals();
        }

        if (!GameManagerScript::s_IsTutorialMode && m_IsInitialized && currentStock > m_LastStockCount) {
            if (m_VisualFood.id != std::numeric_limits<std::size_t>::max()) {
                GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                        m_VisualFood, glm::vec3(1.0f, 0.9f, 0.0f), 0.6f, false
                    });
            }
        }

        m_LastStockCount = currentStock;

        bool isGamepadSquare = Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0);

        if (isGamepadSquare && m_IsHovered && m_SpawnCooldown <= 0.0f && !Input::IsUICapturingMouse() && !MachineScript::GlobalIsMachineHeld) {
            HandleClick();
        }

        if (!m_IsHovered) {
            m_WasHovered = false;
        }

        m_IsHovered = false;
    }

    void OnHoverCursor() override
    {
        if (!m_WasHovered) {
            AudioEngine::PlayLoopingSound("CookingStation/Assets/sounds/hover_in_game.mp3", 0.15f, false);
            m_WasHovered = true;
        }

        m_IsHovered = true;

        if (m_HasStock && m_VisualFood.id != std::numeric_limits<std::size_t>::max())
        {
            GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                    m_VisualFood, glm::vec3(1.0f, 0.9f, 0.0f), 0.0f, true
                });
        }
    }

private:

    std::size_t m_ClickSubId = 0;
    int m_LastStockCount = 0;
    bool m_IsHovered = false;

    void TriggerErrorHighlight()
    {
        GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
            m_Entity, glm::vec3(1.0f, 0.0f, 0.0f), 0.4f, false
            });
    }

    void UpdateVisuals()
    {
        if (m_HasStock) {
            if (m_VisualFood.id == std::numeric_limits<std::size_t>::max()) {
                SpawnVisualFoodInsideCrate();
            }
        }
        else {
            if (m_VisualFood.id != std::numeric_limits<std::size_t>::max()) {
                GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_VisualFood });
                m_VisualFood = { std::numeric_limits<std::size_t>::max(), 0 };
            }
        }
    }

    std::string GetModelPath(IngredientType type)
    {
        switch (type) {
        case IngredientType::Tomato: return "assets://models/skladniki/pomidor/pomidor.gltf";
        case IngredientType::Cheese: return "assets://models/skladniki/ser/ser.gltf";
        case IngredientType::Ham: return "assets://models/skladniki/szynka/szynka.gltf";
        case IngredientType::Baguette: return "assets://models/skladniki/bagietka/bagietka.gltf";
        case IngredientType::Milk: return "assets://models/skladniki/mleko/milk.gltf";
        case IngredientType::Flour: return "assets://models/skladniki/maka/maka.gltf";
        case IngredientType::Egg: return "assets://models/skladniki/jajko_w_skorupce/egg_withshell.gltf";
        case IngredientType::Mozzarella: return "assets://models/skladniki/pomidor/mozzarella.gltf";
        case IngredientType::Apple: return "assets://models/skladniki/jablko/apple1.gltf";
        case IngredientType::Raspberry: return "assets://models/skladniki/malina/malina.gltf";
        case IngredientType::Strawberry: return "assets://models/skladniki/truskawka/strawberry.gltf";
        case IngredientType::CoffeeBeans: return "assets://models/skladniki/napoje/ziarnokawy.gltf";
        case IngredientType::SleepyDust: return "assets://models/skladniki/pyl/pyl.gltf";
        case IngredientType::Yawn: return "assets://models/skladniki/ziewniecie/ziewniecie.gltf"; 
        case IngredientType::Potato: return "assets://models/skladniki/ziemniak/potato2.gltf";
        default: return "";
        }
    }

    void SpawnVisualFoodInsideCrate()
    {
        std::string modelPath = GetModelPath(m_CrateIngredient);
        if (modelPath.empty()) return;

        auto builder = GetScene()->GetWorld().BuildEntity();
        builder.With<TagComponent>({ "Crate_Visual_Item" });

        TransformComponent tc;
        IngredientMetadata meta = GetIngredientMetadata(m_CrateIngredient);
        tc.SetScale(meta.scale * 0.7f);
        tc.SetPosition(GetComponent<TransformComponent>()->GetPosition() + glm::vec3(0.0f, 0.4f, 0.0f) + meta.offset);
        tc.SetRotation(meta.rotation);
        builder.With<TransformComponent>(tc);

        MeshComponent mesh;
        mesh.ModelPtr = AssetManager::GetModel(modelPath);
        builder.With<MeshComponent>(mesh);

        m_VisualFood = builder.Build();
    }

    bool SpawnIngredientOnConveyor()
    {
        // ... (reszta Twojej oryginalnej metody SpawnIngredientOnConveyor - jest bezbłędna)
        Entity closestConveyor = { std::numeric_limits<std::size_t>::max(), 0 };
        float closestDist = 3.5f;
        glm::vec3 spawnPos = GetComponent<TransformComponent>()->GetPosition();

        ConveyorScript* targetConvScript = nullptr;

        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();

        if (scripts && transforms) {
            for (size_t i = 0; i < scripts->dense.size(); ++i) {
                auto& nsc = scripts->dense[i];
                for (auto& s : nsc.Scripts) {
                    if (s.Name == "ConveyorScript") {
                        Entity conveyorEntity = scripts->reverse[i];
                        auto* conveyorTf = transforms->Get(conveyorEntity);
                        if (conveyorTf) {
                            float dist = glm::distance(GetComponent<TransformComponent>()->GetPosition(), conveyorTf->GetPosition());
                            if (dist < closestDist) {
                                closestDist = dist;
                                closestConveyor = conveyorEntity;
                                spawnPos = conveyorTf->GetPosition();
                                spawnPos.y += 1.3f;
                                targetConvScript = static_cast<ConveyorScript*>(s.Instance);
                            }
                        }
                        break;
                    }
                }
            }
        }

        if (closestConveyor.id != std::numeric_limits<std::size_t>::max() && targetConvScript) {

            if (targetConvScript->IsOccupied) {
                spdlog::warn("Skrzynka: Tasmociag jest wlasnie zajmowany przez wjezdzajacy obiekt!");
                return false;
            }

            bool isOccupied = false;
            for (size_t i = 0; i < transforms->dense.size(); ++i) {
                Entity e = transforms->reverse[i];

                if (e.id == m_Entity.id || e.id == m_VisualFood.id || e.id == closestConveyor.id) continue;

                glm::vec3 otherPos = transforms->dense[i].GetPosition();

                if (glm::distance(glm::vec2(otherPos.x, otherPos.z), glm::vec2(spawnPos.x, spawnPos.z)) < 1.2f) {
                    auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(e);
                    auto* tag = GetScene()->GetWorld().GetComponent<TagComponent>(e);

                    bool isItem = false;

                    if (tag && (tag->Tag.find("BeltItem") != std::string::npos || tag->Tag.find("Plate") != std::string::npos)) {
                        isItem = true;
                    }

                    if (nsc) {
                        for (auto& script : nsc->Scripts) {
                            if (script.Name == "ItemScript" || script.Name == "PlateScript") {
                                isItem = true;
                                break;
                            }
                        }
                    }

                    if (isItem) {
                        isOccupied = true;
                        break;
                    }
                }
            }

            if (isOccupied) {
                spdlog::warn("Skrzynka: Tasmociag jest fizycznie zajety!");
                return false;
            }

            targetConvScript->IsOccupied = true;
            spdlog::info("Skrzynka: Pomyslnie wyrzucono skladnik na tasmociag!");

            auto builder = GetScene()->GetWorld().BuildEntity();
            builder.With<TagComponent>({ "BeltItem_" + std::to_string((int)m_CrateIngredient) });

            TransformComponent tc;
            IngredientMetadata meta = GetIngredientMetadata(m_CrateIngredient);
            tc.SetScale(meta.scale);
            tc.SetRotation(meta.rotation);
            // Dodajemy offset początkowy, żeby obiekt zespawnował się niżej
            tc.SetPosition(spawnPos + meta.offset);
            builder.With<TransformComponent>(tc);

            MeshComponent mesh;
            mesh.ModelPtr = AssetManager::GetModel(GetModelPath(m_CrateIngredient));
            builder.With<MeshComponent>(mesh);

            BoxColliderComponent collider;
            collider.Size = glm::vec3(0.5f) / meta.scale;
            // Magia fizyki: odwracamy offset dla kolajdera.
            // Dzięki temu fizyka kładzie niewidzialny blok na taśmie, wciskając model graficzny idealnie w dół.
            collider.Offset = -meta.offset / meta.scale;
            builder.With<BoxColliderComponent>(collider);

            NativeScriptComponent nsc;
            nsc.AddScript<ItemScript>("ItemScript");
            builder.With<NativeScriptComponent>(nsc);

            builder.Build();
            return true;
        }
        else {
            spdlog::warn("Skrzynka: Nie wykryto w poblizu zadnego tasmociagu!");
            return false;
        }
    }
};