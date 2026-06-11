#pragma once
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Scene/Entity.h"
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scripts/ScriptRegistry.h"
#include "SceneSerializer.h"

class PrefabSerializer {
public:
    inline static uint32_t s_PrefabSpawnCounter = 0;

    // ZAPIS: Wyciagamy komponenty z wybranej encji i wrzucamy do JSONa
    static void Serialize(Scene* scene, Entity entity, const std::string& filepath) {
        nlohmann::json item;
        auto& world = scene->GetWorld();

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
                    item["scripts"] = scriptNames;
                }
            }
        }

        std::string physicalPath = filepath;
        if (filepath.rfind("assets://", 0) == 0) {
            physicalPath = "CookingStation/Assets/" + filepath.substr(9);
        }

        std::ofstream file(physicalPath);
        if (file.is_open()) {
            file << item.dump(4);
            spdlog::info("Zapisano prefab: {}", physicalPath);
        }
        else {
            spdlog::error("Blad: Nie udalo sie zapisac prefaba do pliku: {}", physicalPath);
        }
    }

    // ODCZYT: Teraz zwracamy WEKTOR wszystkich załadowanych encji!
    static std::vector<Entity> Deserialize(Scene* scene, const std::string& filepath, const glm::vec3& spawnPos) {

        s_PrefabSpawnCounter++;
        std::string idSuffix = "_" + std::to_string(s_PrefabSpawnCounter);

        std::string vfsPath = filepath;
        std::replace(vfsPath.begin(), vfsPath.end(), '\\', '/');

        size_t pos = vfsPath.find("Assets/");
        if (pos != std::string::npos) {
            vfsPath = "assets://" + vfsPath.substr(pos + 7);
        }

        std::vector<uint8_t> fileData = VFS::ReadFile(vfsPath);
        if (fileData.empty()) {
            spdlog::error("[PrefabSerializer] Blad VFS!");
            spdlog::error("   -> Oryginal: {}", filepath);
            spdlog::error("   -> Przerobiona: {}", vfsPath);
            return {}; // Zwracamy pusty wektor
        }

        nlohmann::json parsedData = nlohmann::json::parse(fileData.begin(), fileData.end());
        if (parsedData.is_object()) {
            parsedData = nlohmann::json::array({ parsedData });
        }

        std::unordered_map<int, Entity> localIdToRealEntity;
        std::unordered_map<std::size_t, Entity> rawIdToEntity;

        std::vector<Entity> createdEntities; // Tabela na wszystkie stworzone elementy
        auto& world = scene->GetWorld();

        for (const auto& item : parsedData) {
            auto builder = scene->GetWorld().BuildEntity();

            std::string name = item.contains("name") ? item["name"].get<std::string>() : "Prefab";
            std::string nameWithId = name + idSuffix;
            builder.With<TagComponent>({ nameWithId });

            TransformComponent transComp;
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

            if (item.contains("rotation")) {
                transComp.SetRotation({ item["rotation"][0], item["rotation"][1], item["rotation"][2] });
            }

            if (item.contains("scale")) {
                transComp.SetScale({ item["scale"][0], item["scale"][1], item["scale"][2] });
            }
            builder.With<TransformComponent>(transComp);

            std::shared_ptr<Model> model = nullptr;
            if (item.contains("model_path")) {
                std::string path = item["model_path"];
                MeshComponent meshComp;
                model = AssetManager::GetModel(path);
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

            if (model) {
                AnimatorComponent animComp;
                if (SceneSerializer::ParseAnimatorFromJson(item, model, animComp)) {
                    builder.With<AnimatorComponent>(animComp);
                }
            }

            Entity newEntity = builder.Build();

            if (item.contains("local_id")) {
                localIdToRealEntity[item["local_id"].get<int>()] = newEntity;
            }
            rawIdToEntity[newEntity.id] = newEntity;

            createdEntities.push_back(newEntity); // Zapisujemy każdą encję do naszego wektora
        }

        // Pętla budująca strukturę hierarchii (opcjonalna)
        for (const auto& item : parsedData) {
            if (item.contains("parent_id") && item.contains("local_id")) {
                int localId = item["local_id"].get<int>();
                int parentId = item["parent_id"].get<int>();

                Entity child = localIdToRealEntity[localId];
                Entity parent = localIdToRealEntity[parentId];

                auto* childRel = world.GetComponent<RelationshipComponent>(child);
                if (!childRel) {
                    world.AddComponent<RelationshipComponent>(child, RelationshipComponent{});
                    childRel = world.GetComponent<RelationshipComponent>(child);
                }

                auto* parentRel = world.GetComponent<RelationshipComponent>(parent);
                if (!parentRel) {
                    world.AddComponent<RelationshipComponent>(parent, RelationshipComponent{});
                    parentRel = world.GetComponent<RelationshipComponent>(parent);
                }

                childRel->Parent = parent.id;
                parentRel->ChildrenCount++;

                if (parentRel->FirstChild == NULL_ENTITY) {
                    parentRel->FirstChild = child.id;
                }
                else {
                    std::size_t currSiblingId = parentRel->FirstChild;
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

        // Zwracamy caly wektor zbudowanych encji (Garnkow, Palnikow, Szafek itd.)
        return createdEntities;
    }
};