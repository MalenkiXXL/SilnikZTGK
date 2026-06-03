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
    // ZMIANA: Teraz domyœlnym stanem jest None! 
    IngredientType m_CrateIngredient = IngredientType::None;

    Entity m_VisualFood = { std::numeric_limits<std::size_t>::max(), 0 };
    float m_SpawnCooldown = 0.0f;
    bool m_HasStock = false;
    bool m_IsInitialized = false;

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
            else if (name.find("Flour") != std::string::npos || name.find("Maka") != std::string::npos || name.find("M¹ka") != std::string::npos)
                m_CrateIngredient = IngredientType::Flour;
        }

        if (m_CrateIngredient == IngredientType::None) {
            spdlog::error("Skrzynka o ID {} ma nierozpoznany tag! Jest pusta i nie bedzie dzialac.", m_Entity.id);
        }
    }

    void OnDestroy() override
    {
        if (m_VisualFood.id != std::numeric_limits<std::size_t>::max())
            GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_VisualFood });
    }

    void OnUpdate(Timestep ts) override
    {
        // Jeœli skrzynka jest uszkodzona/nie ma przypisanego typu, nie robimy nic
        if (m_CrateIngredient == IngredientType::None) return;

        if (m_SpawnCooldown > 0.0f) {
            m_SpawnCooldown -= ts.GetSeconds();
        }

        // =========================================================
        // SPRAWDZANIE INWENTARZA
        // =========================================================
        int currentStock = GameManagerScript::s_Instance ? GameManagerScript::s_Instance->GetIngredientCount(m_CrateIngredient) : 0;
        bool shouldHaveStock = (currentStock > 0);

        if (!m_IsInitialized || shouldHaveStock != m_HasStock) {
            m_HasStock = shouldHaveStock;
            m_IsInitialized = true;
            UpdateVisuals();
        }

        auto* tf = GetComponent<TransformComponent>();
        if (!tf) return;

        glm::vec3 mousePos = GetMouseWorldPosition();
        glm::vec2 mouse2D = { mousePos.x, mousePos.z };
        glm::vec2 crate2D = { tf->GetPosition().x, tf->GetPosition().z };

        // Powiêkszony, wygodny dystans klikania
        if (glm::distance(mouse2D, crate2D) < 1.2f)
        {
            if (Input::IsMouseButtonJustPressed(0) && m_SpawnCooldown <= 0.0f && !MachineScript::GlobalIsHoveringUI && !MachineScript::GlobalIsMachineHeld)
            {
                // ZMIANA: Filtr wykluczaj¹cy podwójne klikanie skrzynek stoj¹cych obok siebie
                if (IsClosestCrate(mouse2D))
                {
                    if (m_HasStock)
                    {
                        m_SpawnCooldown = 0.2f;
                        SpawnIngredientOnConveyor();
                        GetScene()->GetWorld().GetEventBus().Publish(IngredientUsedEvent{ m_CrateIngredient, 1 });
                    }
                    else
                    {
                        spdlog::warn("Skrzynka: Brak zapasow tego skladnika w magazynie (0 sztuk)!");
                    }
                }
            }
        }
    }

private:

    // Funkcja weryfikuj¹ca, czy myszka nie jest przypadkiem bli¿ej innej skrzynki
    bool IsClosestCrate(glm::vec2 mousePos2D)
    {
        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();
        if (!scripts || !transforms) return true;

        float myDist = glm::distance(mousePos2D, glm::vec2(GetComponent<TransformComponent>()->GetPosition().x, GetComponent<TransformComponent>()->GetPosition().z));

        for (size_t i = 0; i < scripts->dense.size(); ++i) {
            auto& nsc = scripts->dense[i];
            for (auto& s : nsc.Scripts) {
                if (s.Name == "CrateScript") {
                    Entity otherEntity = scripts->reverse[i];
                    if (otherEntity.id == m_Entity.id) continue;

                    auto* tf = transforms->Get(otherEntity);
                    if (tf) {
                        float otherDist = glm::distance(mousePos2D, glm::vec2(tf->GetPosition().x, tf->GetPosition().z));
                        // Jeœli inna skrzynka jest bli¿ej kursora ni¿ my, zwracamy fa³sz
                        if (otherDist < myDist) {
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    }

    void UpdateVisuals()
    {
        auto* crateMesh = GetComponent<MeshComponent>();
        if (crateMesh) {
            if (m_HasStock) {
                // Odkomentuj sposób, w jaki przywracacie normalny kolor modelu w Waszym silniku
                // crateMesh->Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); 
                // crateMesh->ShaderName = "ModelShader";
            }
            else {
                // Odkomentuj sposób, w jaki "szarzycie" model w Waszym silniku
                // crateMesh->Color = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f); 
                // crateMesh->ShaderName = "GrayShader"; 
            }
        }

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
        tc.SetPosition(GetComponent<TransformComponent>()->GetPosition() + glm::vec3(0.0f, 0.4f, 0.0f));
        tc.SetRotation(meta.rotation);
        builder.With<TransformComponent>(tc);

        MeshComponent mesh;
        mesh.ModelPtr = AssetManager::GetModel(modelPath);
        builder.With<MeshComponent>(mesh);

        m_VisualFood = builder.Build();
    }

    void SpawnIngredientOnConveyor()
    {
        Entity closestConveyor = { std::numeric_limits<std::size_t>::max(), 0 };
        float closestDist = 3.5f;
        glm::vec3 spawnPos = GetComponent<TransformComponent>()->GetPosition();

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

                                // ZMIANA: Sk³adnik podniesiony wy¿ej na taœmie, ¿eby nie by³ zatopiony w modelu
                                spawnPos.y += 1.3f;
                            }
                        }
                        break;
                    }
                }
            }
        }

        if (closestConveyor.id != std::numeric_limits<std::size_t>::max()) {
            spdlog::info("Skrzynka: Pomyslnie wyrzucono skladnik na tasmociag!");

            auto builder = GetScene()->GetWorld().BuildEntity();

            // ZMIANA: Przypinamy typ sk³adnika jako String w Tagnam, aby system wiedzia³, co naje¿d¿a z taœmy
            builder.With<TagComponent>({ "BeltItem_" + std::to_string((int)m_CrateIngredient) });

            TransformComponent tc;
            IngredientMetadata meta = GetIngredientMetadata(m_CrateIngredient);
            tc.SetScale(meta.scale);
            tc.SetRotation(meta.rotation);
            tc.SetPosition(spawnPos);
            builder.With<TransformComponent>(tc);

            MeshComponent mesh;
            mesh.ModelPtr = AssetManager::GetModel(GetModelPath(m_CrateIngredient));
            builder.With<MeshComponent>(mesh);

            BoxColliderComponent collider;
            collider.Size = glm::vec3(0.5f);
            builder.With<BoxColliderComponent>(collider);

            NativeScriptComponent nsc;
            nsc.AddScript<ItemScript>("ItemScript");
            builder.With<NativeScriptComponent>(nsc);

            builder.Build();
        }
        else {
            spdlog::warn("Skrzynka: Nie wykryto w poblizu zadnego tasmociagu!");
        }
    }
};