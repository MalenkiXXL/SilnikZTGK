#pragma once
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Scene/Entity.h"
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scripts/RotationScript.h"
#include "CookingStation/Scripts/ConveyorScript.h"
#include "CookingStation/Scripts/ItemScript.h"
#include "CookingStation/Scripts/BeltVisualScript.h"

class PrefabSerializer {
public:
    // ZAPIS: Wyci¹gamy komponenty z wybranej encji i wrzucamy do JSONa
    static void Serialize(Scene* scene, Entity entity, const std::string& filepath) {
        nlohmann::json item;
        auto& world = scene->GetWorld();

        // U¿ywamy bezpieczniejszego GetVector() - dok³adnie jak w SceneSerializerze!
        auto* tagStorage = world.GetComponentVector<TagComponent>();
        auto* transformStorage = world.GetComponentVector<TransformComponent>();
        auto* meshStorage = world.GetComponentVector<MeshComponent>();
        auto* colliderStorage = world.GetComponentVector<BoxColliderComponent>();
        auto* scriptStorage = world.GetComponentVector<NativeScriptComponent>();

        if (tagStorage) {
            if (auto* tag = tagStorage->Get(entity))
                item["name"] = tag->Tag;
        }

        if (transformStorage) {
            if (auto* transform = transformStorage->Get(entity)) {
                // ZMIANA: Pobieramy dane przez bezpieczne Gettery
                glm::vec3 pos = transform->GetPosition();
                glm::vec3 rot = transform->GetRotation();
                glm::vec3 scale = transform->GetScale();

                item["position"] = { pos.x, pos.y, pos.z };
                item["rotation"] = { rot.x, rot.y, rot.z };
                item["scale"] = { scale.x, scale.y, scale.z };
            }
        }

        if (meshStorage) {
            if (auto* mesh = meshStorage->Get(entity)) {
                if (mesh->ModelPtr) item["model_path"] = mesh->ModelPtr->FilePath;
            }
        }

        if (colliderStorage) {
            if (auto* collider = colliderStorage->Get(entity)) {
                item["collider"]["size"] = { collider->Size.x, collider->Size.y, collider->Size.z };
                item["collider"]["offset"] = { collider->Offset.x, collider->Offset.y, collider->Offset.z };
            }
        }

        if (scriptStorage) {
            if (auto* script = scriptStorage->Get(entity)) {
                if (script->InstantiateScript && !script->ScriptName.empty()) {
                    item["script"] = script->ScriptName;
                }
            }
        }

        std::ofstream file(filepath);
        if (file.is_open()) {
            file << item.dump(4);
            spdlog::info("Zapisano prefab: {}", filepath);
        }
    }

    // ODCZYT: Tworzymy now¹ encjê, wczytujemy dane i wymuszamy spawnPos
    static Entity Deserialize(Scene* scene, const std::string& filepath, const glm::vec3& spawnPos) {
        std::ifstream file(filepath);
        if (!file.is_open()) return { std::numeric_limits<std::size_t>::max(), 0 };

        nlohmann::json item = nlohmann::json::parse(file);
        auto builder = scene->GetWorld().BuildEntity();

        std::string name = item.contains("name") ? item["name"].get<std::string>() : "Prefab";
        builder.With<TagComponent>({ name });

        TransformComponent transComp;

        // ZMIANA: Wgrywamy dane przez Settery, co automatycznie ustawi flagê m_IsDirty na true!
        transComp.SetPosition(spawnPos);

        if (item.contains("rotation") && item.contains("scale")) {
            transComp.SetRotation({ item["rotation"][0], item["rotation"][1], item["rotation"][2] });
            transComp.SetScale({ item["scale"][0], item["scale"][1], item["scale"][2] });
        }
        builder.With<TransformComponent>(transComp);

        if (item.contains("model_path")) {
            std::string path = item["model_path"];
            MeshComponent meshComp;
            meshComp.ModelPtr = AssetManager::GetModel(path);
            meshComp.ShaderPtr = AssetManager::GetShaders().Get("ModelShader");
            meshComp.Path = path;
            builder.With<MeshComponent>(meshComp);
        }

        if (item.contains("collider")) {
            BoxColliderComponent bc;
            bc.Size = { item["collider"]["size"][0], item["collider"]["size"][1], item["collider"]["size"][2] };
            bc.Offset = { item["collider"]["offset"][0], item["collider"]["offset"][1], item["collider"]["offset"][2] };
            builder.With<BoxColliderComponent>(bc);
        }

        if (item.contains("script")) {
            NativeScriptComponent nsc;
            std::string scriptName = item["script"].get<std::string>();
            if (scriptName == "RotationScript") nsc.Bind<RotationScript>(scriptName);
            else if (scriptName == "ConveyorScript") nsc.Bind<ConveyorScript>(scriptName);
            else if (scriptName == "BeltVisualScript") nsc.Bind<BeltVisualScript>(scriptName);
            else if (scriptName == "ItemScript") nsc.Bind<ItemScript>(scriptName);

            builder.With<NativeScriptComponent>(nsc);
        }

        return builder.Build();
    }
};