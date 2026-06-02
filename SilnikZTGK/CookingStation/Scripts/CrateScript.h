#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scripts/Managers/IngredientType.h"
#include "CookingStation/Scripts/Plates/ItemScript.h" 
#include "CookingStation/Scripts/Machines/MachineScript.h" 
#include <glm/glm.hpp>
#include <limits>
#include <string>

class CrateScript : public ScriptableEntity
{
public:
    IngredientType m_CrateIngredient = IngredientType::Tomato;

    void OnCreate() override
    {
        // Sprawdzamy nazwê skrzynki z JSON-a
        auto* tagComp = GetComponent<TagComponent>();
        if (tagComp) {
            std::string name = tagComp->Tag;
            if (name.find("Tomato") != std::string::npos || name.find("Pomidor") != std::string::npos)
                m_CrateIngredient = IngredientType::Tomato;
            else if (name.find("Cheese") != std::string::npos || name.find("Ser") != std::string::npos)
                m_CrateIngredient = IngredientType::Cheese;
            else if (name.find("Ham") != std::string::npos || name.find("Szynka") != std::string::npos)
                m_CrateIngredient = IngredientType::Ham;
            else if (name.find("Milk") != std::string::npos || name.find("Mleko") != std::string::npos)
                m_CrateIngredient = IngredientType::Milk;
            else if (name.find("Flour") != std::string::npos || name.find("Maka") != std::string::npos)
                m_CrateIngredient = IngredientType::Flour;
        }
    }

    void OnUpdate(Timestep ts) override
    {
        auto* tf = GetComponent<TransformComponent>();
        if (!tf) return;

        glm::vec3 mousePos = GetMouseWorldPosition();
        glm::vec2 mouse2D = { mousePos.x, mousePos.z };
        glm::vec2 crate2D = { tf->GetPosition().x, tf->GetPosition().z };

        // Jeœli kursor jest blisko skrzynki i klikamy
        if (glm::distance(mouse2D, crate2D) < 2.0f)
        {
            if (Input::IsMouseButtonJustPressed(0) && !MachineScript::GlobalIsHoveringUI && !MachineScript::GlobalIsMachineHeld)
            {
                SpawnIngredientOnConveyor();
            }
        }
    }

private:
    std::string GetModelPath(IngredientType type)
    {
        switch (type) {
        case IngredientType::Tomato: return "assets://models/warzywka/pomidor/pomidor.gltf";
        case IngredientType::Cheese: return "assets://models/skladniki/ser/ser.gltf";
        case IngredientType::Ham: return "assets://models/skladniki/szynka/szynka.gltf";;
        case IngredientType::Milk: return "assets://models/skladniki/mleko/milk.gltf";
        case IngredientType::Flour: return "assets://models/skladniki/maka/maka.gltf";
        default: return "";
        }
    }

    void SpawnIngredientOnConveyor()
    {
        Entity closestConveyor = { std::numeric_limits<std::size_t>::max(), 0 };
        float closestDist = 3.5f;
        glm::vec3 spawnPos = GetComponent<TransformComponent>()->GetPosition();

        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();

        // Szukamy taœmoci¹gu w okolicy naszej skrzynki
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
                                spawnPos.y += 0.8f; // Zrzucamy sk³adnik nad siatkê taœmy
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
            builder.With<TagComponent>({ "SurowySkladnik" });

            TransformComponent tc;
            IngredientMetadata meta = GetIngredientMetadata(m_CrateIngredient);
            tc.SetScale(meta.scale);
            tc.SetRotation(meta.rotation);
            tc.SetPosition(spawnPos);
            builder.With<TransformComponent>(tc);

            MeshComponent mesh;
            mesh.ModelPtr = AssetManager::GetModel(GetModelPath(m_CrateIngredient));
            builder.With<MeshComponent>(mesh);

            // Fizyka niezbêdna do interakcji w grze
            BoxColliderComponent collider;
            collider.Size = glm::vec3(1.2f);
            builder.With<BoxColliderComponent>(collider);

            // Podpiêcie ruchu po taœmie (ItemScript)
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