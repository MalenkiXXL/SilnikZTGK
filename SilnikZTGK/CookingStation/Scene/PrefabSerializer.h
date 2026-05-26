#pragma once
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Scene/Entity.h"
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scripts/ScriptRegistry.h"

class PrefabSerializer {
public:
    inline static uint32_t s_PrefabSpawnCounter = 0;
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

        s_PrefabSpawnCounter++;
        std::string idSuffix = "_" + std::to_string(s_PrefabSpawnCounter);

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
        nlohmann::json parsedData = nlohmann::json::parse(fileData.begin(), fileData.end());
        if (parsedData.is_object()) {
            parsedData = nlohmann::json::array({ parsedData });
        }

        std::unordered_map<int, Entity> localIdToRealEntity;
        std::unordered_map<std::size_t, Entity> rawIdToEntity;
        Entity rootEntity = { std::numeric_limits<std::size_t>::max(), 0 };
        auto& world = scene->GetWorld();

        for (const auto& item : parsedData) {
            auto builder = scene->GetWorld().BuildEntity();

        std::string name = item.contains("name") ? item["name"].get<std::string>() : "Prefab";
        std::string nameWithId = name + idSuffix;
        builder.With<TagComponent>({ nameWithId });

        TransformComponent transComp;

        // ZMIANA: Wgrywamy dane przez Settery, co automatycznie ustawi flag� m_IsDirty na true!
        transComp.SetPosition(spawnPos);

        glm::vec3 localPos = { 0.0f, 0.0f, 0.0f };
        if (item.contains("position")) {
            localPos = { item["position"][0], item["position"][1], item["position"][2] };
        }

        if (item.contains("parent_id")) {
            transComp.SetPosition(localPos);
        }
        else {
            transComp.SetPosition(spawnPos + localPos);
        }

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
            for (const auto& scriptName : item["scripts"]) {
                ScriptRegistry::AddScriptToComponent(nsc, scriptName.get<std::string>());
            }
            builder.With<NativeScriptComponent>(nsc);
        }
        else if (item.contains("script")) {
            NativeScriptComponent nsc;
            ScriptRegistry::AddScriptToComponent(nsc, item["script"].get<std::string>());
            builder.With<NativeScriptComponent>(nsc);
        }

        Entity newEntity = builder.Build();

            if (item.contains("local_id")) {
                localIdToRealEntity[item["local_id"].get<int>()] = newEntity;
            }
            rawIdToEntity[newEntity.id] = newEntity;

            if (rootEntity.id == std::numeric_limits<std::size_t>::max()) {
                rootEntity = newEntity;
            }
        }

        // --- ZMIANA 6: Druga pętla budująca strukturę hierarchii (opcjonalna) ---
        for (const auto& item : parsedData) {
            if (item.contains("parent_id") && item.contains("local_id")) {
                int localId = item["local_id"].get<int>();
                int parentId = item["parent_id"].get<int>();

                Entity child = localIdToRealEntity[localId];
                Entity parent = localIdToRealEntity[parentId];

                // 1. Zapewniamy istnienie komponentu Relationship na dziecku
                auto* childRel = world.GetComponent<RelationshipComponent>(child);
                if (!childRel) {
                    world.AddComponent<RelationshipComponent>(child, RelationshipComponent{});
                    childRel = world.GetComponent<RelationshipComponent>(child);
                }

                // 2. Zapewniamy istnienie komponentu Relationship na rodzicu
                auto* parentRel = world.GetComponent<RelationshipComponent>(parent);
                if (!parentRel) {
                    world.AddComponent<RelationshipComponent>(parent, RelationshipComponent{});
                    parentRel = world.GetComponent<RelationshipComponent>(parent);
                }

                // 3. Ustawiamy powiązanie w górę (dziecko -> rodzic)
                childRel->Parent = parent.id;
                parentRel->ChildrenCount++;

                // 4. Ustawiamy powiązania w dół i w bok (rodzic -> dzieci, brat -> brat)
                if (parentRel->FirstChild == NULL_ENTITY) {
                    // Jeśli to pierwsze dziecko, przypisujemy je bezpośrednio do ojca
                    parentRel->FirstChild = child.id;
                }
                else {
                    std::size_t currSiblingId = parentRel->FirstChild;
                    // Tłumaczymy surowe ID na pełną encję za pomocą naszej nowej mapy!
                    Entity currSibling = rawIdToEntity[currSiblingId];
                    auto* currSiblingRel = world.GetComponent<RelationshipComponent>(currSibling);

                    while (currSiblingRel->NextSibling != NULL_ENTITY) {
                        currSiblingId = currSiblingRel->NextSibling;
                        currSibling = rawIdToEntity[currSiblingId];
                        currSiblingRel = world.GetComponent<RelationshipComponent>(currSibling);
                    }

                    currSiblingRel->NextSibling = child.id;
                    childRel->PreviousSibling = currSiblingId;
                }
            }
        }

        return rootEntity;

    } // Koniec funkcji Deserialize
}; // Koniec klasy PrefabSerializer