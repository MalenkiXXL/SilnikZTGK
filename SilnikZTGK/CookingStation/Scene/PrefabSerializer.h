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
            std::ifstream file(filepath, std::ios::ate | std::ios::binary);
            if (file.is_open()) {
                size_t fileSize = (size_t)file.tellg();
                fileData.resize(fileSize);
                file.seekg(0);
                file.read((char*)fileData.data(), fileSize);
                file.close();
            }
        }

        if (fileData.empty()) {
            spdlog::error("[PrefabSerializer] Blad VFS!");
            spdlog::error("   -> Oryginal: {}", filepath);
            spdlog::error("   -> Przerobiona: {}", vfsPath);
            return {}; 
        }

        nlohmann::json parsedData = nlohmann::json::parse(fileData.begin(), fileData.end());

        if (parsedData.is_object()) {
            if (parsedData.contains("entities")) {
                parsedData = parsedData["entities"];
            }
            else if (parsedData.contains("Entities")) {
                parsedData = parsedData["Entities"];
            }
            else {
                parsedData = nlohmann::json::array({ parsedData });
            }
        }
        else if (parsedData.is_object()) {
            parsedData = nlohmann::json::array({ parsedData });
        }

        std::unordered_map<int, Entity> localIdToRealEntity;
        std::unordered_map<std::size_t, Entity> rawIdToEntity;

        std::vector<Entity> createdEntities; 
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
                std::string path = item["model_path"].get<std::string>();

                size_t assetPos = path.find("Assets/");
                if (assetPos != std::string::npos) {
                    path = "assets://" + path.substr(assetPos + 7);
                }

                MeshComponent meshComp;
                model = AssetManager::GetModel(path);
                meshComp.ModelPtr = model;
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

            if (model && item.contains("animator")) {
                if (item["animator"].contains("type") && item["animator"]["type"] == "transform") {
                    TransformAnimatorComponent transAnim;
                    transAnim.IsPlaying = item["animator"].contains("is_playing") ? item["animator"]["is_playing"].get<bool>() : false;

                    if (item["animator"].contains("clips")) {
                        for (const auto& [clipName, clipPath] : item["animator"]["clips"].items()) {
                            transAnim.Animations[clipName] = std::make_shared<NodeAnimation>(clipPath.get<std::string>());
                        }
                    }
                    builder.With<TransformAnimatorComponent>(transAnim);
                    spdlog::info("[PrefabSerializer] Sukces: Podpieto TransformAnimatorComponent do encji!");
                } else {
                    AnimatorComponent animComp;
                    if (SceneSerializer::ParseAnimatorFromJson(item, model, animComp)) {
                        builder.With<AnimatorComponent>(animComp);
                    }
                }
            }

            Entity newEntity = builder.Build();

            int entityLocalId = -1;
            if (item.contains("local_id")) entityLocalId = item["local_id"].get<int>();
            else if (item.contains("id")) entityLocalId = item["id"].get<int>();

            if (entityLocalId != -1) {
                localIdToRealEntity[entityLocalId] = newEntity;
            }
            rawIdToEntity[newEntity.id] = newEntity;

            createdEntities.push_back(newEntity);
        }

        for (const auto& item : parsedData) {
            int localId = -1;
            if (item.contains("local_id")) localId = item["local_id"].get<int>();
            else if (item.contains("id")) localId = item["id"].get<int>();

            if (item.contains("parent_id") && localId != -1) {
                int parentId = item["parent_id"].get<int>();

                Entity child = localIdToRealEntity[localId];
                Entity parent = localIdToRealEntity[parentId];

                if (!world.GetComponent<RelationshipComponent>(child)) {
                    world.AddComponent<RelationshipComponent>(child, RelationshipComponent{});
                }
                if (!world.GetComponent<RelationshipComponent>(parent)) {
                    world.AddComponent<RelationshipComponent>(parent, RelationshipComponent{});
                }

                auto* childRel = world.GetComponent<RelationshipComponent>(child);
                auto* parentRel = world.GetComponent<RelationshipComponent>(parent);

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

        return createdEntities;
    }
};