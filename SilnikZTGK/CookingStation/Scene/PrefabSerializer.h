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
#include "CookingStation/Scripts/ScriptRegistry.h"

class PrefabSerializer {
public:
    // ZAPIS: Wyci�gamy komponenty z wybranej encji i wrzucamy do JSONa
    static void Serialize(Scene* scene, Entity entity, const std::string& filepath) {
        nlohmann::json item;
        auto& world = scene->GetWorld();

        // U�ywamy bezpieczniejszego GetVector() - dok�adnie jak w SceneSerializerze!
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
            if (auto* nsc = scriptStorage->Get(entity)) {
                if (!nsc->Scripts.empty()) {
                    std::vector<std::string> scriptNames;
                    for (const auto& s : nsc->Scripts) {
                        scriptNames.push_back(s.Name);
                    }
                    item["scripts"] = scriptNames; // Klucz "scripts" z 's' na ko�cu!
                }
            }
        }

        std::ofstream file(filepath);
        if (file.is_open()) {
            file << item.dump(4);
            spdlog::info("Zapisano prefab: {}", filepath);
        }
    }

    static Entity Deserialize(Scene* scene, const std::string& filepath, const glm::vec3& spawnPos) {
        // --- KULOODPORNY SANITIZER ŚCIEŻEK ---
        std::string vfsPath = filepath;
        std::replace(vfsPath.begin(), vfsPath.end(), '\\', '/'); // Zamiana backslashy na ukośniki

        // Szukamy słowa "Assets/" w dowolnym miejscu ścieżki
        size_t pos = vfsPath.find("Assets/");
        if (pos != std::string::npos) {
            // Ucinamy wszystko do "Assets/" i doklejamy assets://
            vfsPath = "assets://" + vfsPath.substr(pos + 7); // 7 to długość słowa "Assets/"
        }
        // -------------------------------------

        std::vector<uint8_t> fileData = VFS::ReadFile(vfsPath);
        if (fileData.empty()) {
            // Jeśli VFS nadal nie zadziała, ten log powie nam całą prawdę
            spdlog::error("[PrefabSerializer] Blad VFS!");
            spdlog::error("   -> Oryginal: {}", filepath);
            spdlog::error("   -> Przerobiona: {}", vfsPath);
            return { std::numeric_limits<std::size_t>::max(), 0 };
        }

        // Parsowanie bezpośrednio z pamięci RAM (VFS)
        nlohmann::json item = nlohmann::json::parse(fileData.begin(), fileData.end());
        auto builder = scene->GetWorld().BuildEntity();

        std::string name = item.contains("name") ? item["name"].get<std::string>() : "Prefab";
        builder.With<TagComponent>({ name });

        TransformComponent transComp;

        // ZMIANA: Wgrywamy dane przez Settery, co automatycznie ustawi flag� m_IsDirty na true!
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
            meshComp.ShaderPtr = nullptr;
            meshComp.Path = path;
            builder.With<MeshComponent>(meshComp);
        }

        if (item.contains("collider")) {
            BoxColliderComponent bc;
            bc.Size = { item["collider"]["size"][0], item["collider"]["size"][1], item["collider"]["size"][2] };
            bc.Offset = { item["collider"]["offset"][0], item["collider"]["offset"][1], item["collider"]["offset"][2] };
            builder.With<BoxColliderComponent>(bc);
        }

        if (item.contains("scripts")) {
            NativeScriptComponent nsc;
            // Iterujemy po liście w JSON i dodajemy do komponentu przez nasz nowy Rejestr
            for (const auto& scriptName : item["scripts"]) {
                ScriptRegistry::AddScriptToComponent(nsc, scriptName.get<std::string>());
            }
            builder.With<NativeScriptComponent>(nsc);
        }
        // Opcjonalnie: kompatybilność wsteczna z Twoimi starymi zapisami scen
        else if (item.contains("script")) {
            NativeScriptComponent nsc;
            ScriptRegistry::AddScriptToComponent(nsc, item["script"].get<std::string>());
            builder.With<NativeScriptComponent>(nsc);
        }

        return builder.Build();
    } // Koniec funkcji Deserialize
}; // Koniec klasy PrefabSerializer