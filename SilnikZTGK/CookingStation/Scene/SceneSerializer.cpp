#include "SceneSerializer.h"
#include "CookingStation/Scene/Entity.h"
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Layers/AssetLayer/AssetLayer.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "ecs.h"
#include <glm/glm.hpp>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "CookingStation/Scripts/RotationScript.h"
#include "CookingStation/Scripts/ConveyorBelt/ConveyorScript.h"
#include "CookingStation/Scripts/ConveyorBelt/BeltVisualScript.h"
#include "CookingStation/Scripts/Plates/ItemScript.h"
#include "CookingStation/Scripts/PotScript.h"
#include "CookingStation/Scripts/ScriptRegistry.h"
// DODANO NAGŁÓWKI ANIMACJI
#include "CookingStation/Layers/AssetLayer/Animation.h"
#include "CookingStation/Layers/GameLayer/Animator.h"
#include "CookingStation/Scripts/DragAndDropScript.h"
#include "CookingStation/Scripts/CustomerManagerScript.h"


using json = nlohmann::json;

// Wczytuje dane z pliku do pamięci i buduje świat w silniku
bool SceneSerializer::Deserialize(const std::string& path) {
    std::vector<uint8_t> fileData = VFS::ReadFile(path);
    if (fileData.empty()) {
        spdlog::error("[SceneSerializer] Nie udalo sie otworzyc pliku przez VFS: {}", path);
        return false;
    }

    // Parsowanie bezpośrednio z bufora pamięci RAM
    json data = json::parse(fileData.begin(), fileData.end());

    if (data.contains("settings")) {
        auto& settings = data["settings"];
        if (settings.contains("clear_color")) {
            auto& c = settings["clear_color"]; // pobieranie tablicy rgba

            // BEZPIECZNE pobieranie kolorów - jeżeli nie ma alfy (c[3]), dajemy 1.0f
            float r = c[0];
            float g = c[1];
            float b = c[2];
            float a = (c.size() > 3) ? c[3].get<float>() : 1.0f;

            // encja techniczna, przechowujaca kolor czyszczenia 
            m_Scene->GetWorld().BuildEntity()
                .With<ClearColorComponent>({ { r, g, b, a } })
                .Build();
        }
    }

    if (data.contains("entities")) {

        std::unordered_map<std::size_t, Entity> oldToNew;
        std::vector<std::pair<Entity, std::size_t>> pendingRelationships;

        for (auto& item : data["entities"]) {

            std::string name = item.contains("name") ? item["name"].get<std::string>() : "Nowy Obiekt";
            std::string modelPath = item.contains("model_path") ? item["model_path"].get<std::string>() : "";

            auto builder = m_Scene->GetWorld().BuildEntity();
            builder.With<TagComponent>({ name });

            std::shared_ptr<Model> model = nullptr;
            if (!modelPath.empty()) {
                model = AssetManager::GetModel(modelPath);
                MeshComponent meshComp;
                meshComp.ModelPtr = model;
                builder.With<MeshComponent>(meshComp);
            }

            TransformComponent transComp;

            if (item.contains("position") && item.contains("rotation") && item.contains("scale")) {
                transComp.SetPosition({ item["position"][0], item["position"][1], item["position"][2] });
                transComp.SetRotation({ item["rotation"][0], item["rotation"][1], item["rotation"][2] });
                transComp.SetScale({ item["scale"][0], item["scale"][1], item["scale"][2] });
            }
            else {
                transComp.SetPosition({ 0.0f, 0.0f, 0.0f });
                transComp.SetRotation({ 0.0f, 0.0f, 0.0f });
                transComp.SetScale({ 1.0f, 1.0f, 1.0f });
            }
            builder.With<TransformComponent>(transComp);

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
            // Opcjonalnie: kompatybilność wsteczna z Twoimi starymi zapisami scen (stary klucz "script")
            else if (item.contains("script")) {
                NativeScriptComponent nsc;
                ScriptRegistry::AddScriptToComponent(nsc, item["script"].get<std::string>());
                builder.With<NativeScriptComponent>(nsc);
            }


            if (model) {
                auto animator = std::make_shared<Animator>();
                bool hasAnyAnimation = false;

                // 1. Sprawdzamy animację wbudowaną (np. grzybek.gltf)
                auto defaultAnim = std::make_shared<Animation>(modelPath, model.get());
                if (defaultAnim->GetDuration() > 0.0f) {
                    animator->AddAnimation("Default", defaultAnim);
                    hasAnyAnimation = true;
                }

                if (item.contains("animator") && item["animator"].contains("clips")) {
                    for (auto& el : item["animator"]["clips"].items()) {
                        std::string clipName = el.key();
                        std::string clipPath = el.value();

                        // Kluczowa zmiana: osobny model dla pliku animacji
                        auto clipModel = AssetManager::GetModel(clipPath);
                        auto clipAnim = std::make_shared<Animation>(clipPath, clipModel.get());

                        if (clipAnim->GetDuration() > 0.0f) {
                            animator->AddAnimation(clipName, clipAnim);
                            hasAnyAnimation = true;
                        }
                    }
                }

                // 3. Jeśli cokolwiek się załadowało, montujemy komponent
                if (hasAnyAnimation) {
                    AnimatorComponent animComp(animator);

                    if (item.contains("animator")) {
                        // Wczytujemy ustawienia z JSON
                        animComp.IsPlaying = item["animator"].value("is_playing", false);
                        animComp.PlaybackSpeed = item["animator"].value("playback_speed", 1.0f);

                        // KLUCZOWY MOMENT: Ustawiamy kasetę w odtwarzaczu
                        if (item["animator"].contains("start_clip")) {
                            std::string sClip = item["animator"]["start_clip"];
                            // To ustawia m_CurrentAnimation na "Walk" jeszcze przed startem klatki
                            animator->PlayAnimation(sClip);
                        }
                        else {
                            animator->PlayAnimation("Default");
                        }
                    }
                    else {
                        // Domyślne wartości dla obiektów bez wpisu "animator" w JSON
                        animComp.IsPlaying = false;
                        animComp.PlaybackSpeed = 1.0f;
                        animator->PlayAnimation("Default");
                    }

                    builder.With<AnimatorComponent>(animComp);
                }
            }

            // FINALIZACJA BUDOWY
            Entity newEntity = builder.Build();

            // Zapamiętujemy, jakie ID dostał ten obiekt w stosunku do tego w pliku
            if (item.contains("id")) {
                std::size_t oldId = item["id"].get<std::size_t>();
                oldToNew[oldId] = newEntity;
            }

            // Jeśli encja miała w pliku rodzica, dodajemy ją do kolejki 
            if (item.contains("parent_id")) {
                std::size_t oldParentId = item["parent_id"].get<std::size_t>();
                pendingRelationships.push_back({ newEntity, oldParentId });
            }

           // spdlog::info("[SceneLoader] Dodano: {}", name);
        }

        // ODBUDOWA RELACJI
        // Kiedy wszystkie obiekty są już na mapie, możemy je popiąć
        for (auto& pair : pendingRelationships) {
            Entity childEntity = pair.first;
            std::size_t oldParentId = pair.second;

            // Szukamy nowo stworzonego rodzica w słowniku
            if (oldToNew.find(oldParentId) != oldToNew.end()) {
                Entity newParentEntity = oldToNew[oldParentId];
                m_Scene->SetParent(childEntity, newParentEntity);
            }
        }
    }

    // Na końcu SceneSerializer::Deserialize(...)
    m_Scene->RebuildConveyorCache();
    return true; // scena wczytana pomyslnie
}

void SceneSerializer::Serialize(const std::string& filepath) {
    json data;
    auto& world = m_Scene->GetWorld();

    data["entities"] = json::array();

    auto* tagStorage = world.GetComponentVector<TagComponent>();
    if (!tagStorage) return;

    auto* transformStorage = world.GetComponentVector<TransformComponent>();
    auto* meshStorage = world.GetComponentVector<MeshComponent>();
    auto* colliderStorage = world.GetComponentVector<BoxColliderComponent>();
    auto* scriptStorage = world.GetComponentVector<NativeScriptComponent>();
    auto* relStorage = world.GetComponentVector<RelationshipComponent>();
    auto* animatorStorage = world.GetComponentVector<AnimatorComponent>(); // DODANE

    for (size_t i = 0; i < tagStorage->reverse.size(); ++i) {
        Entity entity = tagStorage->reverse[i];
        json item;

        item["id"] = entity.id;
        item["name"] = tagStorage->dense[i].Tag;

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
            auto* mesh = meshStorage->Get(entity);
            if (mesh && mesh->ModelPtr) {
                item["model_path"] = mesh->ModelPtr->FilePath;
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

        // ===============================================
        // ZMIANA: SERIALIZACJA ANIMATORA DO JSON
        // ===============================================
        if (animatorStorage) {
            if (auto* animComp = animatorStorage->Get(entity)) {
                item["animator"]["is_playing"] = animComp->IsPlaying;
                item["animator"]["playback_speed"] = animComp->PlaybackSpeed;

                if (animComp->AnimatorInstance) {
                    // Zapisujemy aktualnie ustawiony klip jako start_clip
                    const std::string& currentClip = animComp->AnimatorInstance->GetCurrentAnimationName();
                    if (!currentClip.empty() && currentClip != "Default") {
                        item["animator"]["start_clip"] = currentClip;
                    }

                    // Zapisujemy wszystkie klipy (z wyjątkiem Default, który odtwarzamy z model_path)
                    json clips = json::object();
                    for (const auto& [clipName, clipAnim] : animComp->AnimatorInstance->GetAnimations()) {
                        if (clipName != "Default") {
                            clips[clipName] = clipAnim->GetPath();
                        }
                    }
                    if (!clips.empty()) {
                        item["animator"]["clips"] = clips;
                    }
                }
            }
        }

        if (relStorage) {
            if (auto* rel = relStorage->Get(entity)) {
                if (rel->Parent != NULL_ENTITY) {
                    item["parent_id"] = rel->Parent;
                }
            }
        }

        data["entities"].push_back(item);
    }

    std::string physicalPath = filepath;
    if (filepath.rfind("assets://", 0) == 0) {
        physicalPath = "CookingStation/Assets/" + filepath.substr(9);
    }

    std::ofstream file(physicalPath);
    if (file.is_open()) {
#ifdef _DEBUG
        file << data.dump(4);
#else
        file << data.dump();
#endif
        spdlog::info("[SceneSerializer] Zapisano scene do: {}", filepath);
    }
    else {
        spdlog::error("[SceneSerializer] Nie udalo sie zapisac pliku {}", filepath);
    }
}