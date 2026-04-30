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
#include "CookingStation/Scripts/ConveyorScript.h"
#include "CookingStation/Scripts/ItemScript.h"

using json = nlohmann::json;

// Wczytuje dane z pliku do pamiêci i buduje œwiat w silniku
bool SceneSerializer::Deserialize(const std::string& path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		spdlog::error("Nie udalo sie otworzyc pliku {}", path);
		return false;
	}

	// parsowanie tekstu na obiekt json
	json data = json::parse(file);

	if (data.contains("settings")) {
		auto& settings = data["settings"];
		if (settings.contains("clear_color")) {
			auto& c = settings["clear_color"]; // pobieranie tablicy rgba

			// BEZPIECZNE pobieranie kolorów - je¿eli nie ma alfy (c[3]), dajemy 1.0f
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

		// NOWE: S³ownik do t³umaczenia "starych" ID z JSONA na "nowe" ID stworzone przez ECS podczas ³adowania
		std::unordered_map<std::size_t, Entity> oldToNew;

		// NOWE: Lista dzieci i starych ID ich rodziców, do po³¹czenia pod koniec
		std::vector<std::pair<Entity, std::size_t>> pendingRelationships;

		for (auto& item : data["entities"]) {

			std::string name = item.contains("name") ? item["name"].get<std::string>() : "Nowy Obiekt";
			std::string modelPath = item.contains("model_path") ? item["model_path"].get<std::string>() : "";

			auto builder = m_Scene->GetWorld().BuildEntity();
			builder.With<TagComponent>({ name });

			if (!modelPath.empty()) {
				auto model = AssetManager::GetModel(modelPath);
				MeshComponent meshComp;
				meshComp.ModelPtr = model;
				builder.With<MeshComponent>(meshComp);
			}

			TransformComponent transComp;
			if (item.contains("position") && item.contains("rotation") && item.contains("scale")) {
				transComp.Position = { item["position"][0], item["position"][1], item["position"][2] };
				transComp.Rotation = { item["rotation"][0], item["rotation"][1], item["rotation"][2] };
				transComp.Scale = { item["scale"][0], item["scale"][1], item["scale"][2] };
			}
			else {
				transComp.Position = { 0.0f, 0.0f, 0.0f };
				transComp.Rotation = { 0.0f, 0.0f, 0.0f };
				transComp.Scale = { 1.0f, 1.0f, 1.0f };
			}
			builder.With<TransformComponent>(transComp);

			if (item.contains("collider")) {
				BoxColliderComponent bc;
				bc.Size = { item["collider"]["size"][0], item["collider"]["size"][1], item["collider"]["size"][2] };
				bc.Offset = { item["collider"]["offset"][0], item["collider"]["offset"][1], item["collider"]["offset"][2] };
				builder.With<BoxColliderComponent>(bc);
			}

			if (item.contains("script")) {
				NativeScriptComponent nsc;
				std::string scriptName = item["script"].get<std::string>();

				if (scriptName == "RotationScript") {
					nsc.Bind<RotationScript>(scriptName);
				}

				else if (scriptName == "ConveyorScript") { 
					nsc.Bind<ConveyorScript>(scriptName);
				}

				else if (scriptName == "ItemScript") {
					nsc.Bind<ItemScript>(scriptName);
				}

				builder.With<NativeScriptComponent>(nsc);
			}



			// FINALIZACJA BUDOWY
			Entity newEntity = builder.Build();

			// Zapamiêtujemy, jakie ID dosta³ ten obiekt w stosunku do tego w pliku
			if (item.contains("id")) {
				std::size_t oldId = item["id"].get<std::size_t>();
				oldToNew[oldId] = newEntity;
			}

			// Jeœli encja mia³a w pliku rodzica, dodajemy j¹ do kolejki 
			if (item.contains("parent_id")) {
				std::size_t oldParentId = item["parent_id"].get<std::size_t>();
				pendingRelationships.push_back({ newEntity, oldParentId });
			}

			spdlog::info("[SceneLoader] Dodano: {}", name);
		}

		// ODBUDOWA RELACJI
		// Kiedy wszystkie obiekty s¹ ju¿ na mapie, mo¿emy je popi¹c
		for (auto& pair : pendingRelationships) {
			Entity childEntity = pair.first;
			std::size_t oldParentId = pair.second;

			// Szukamy nowo stworzonego rodzica w s³owniku
			if (oldToNew.find(oldParentId) != oldToNew.end()) {
				Entity newParentEntity = oldToNew[oldParentId];
				m_Scene->SetParent(childEntity, newParentEntity);
			}
		}
	}

	// Na koñcu SceneSerializer::Deserialize(...)
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
	// NOWE: Pobieramy relacje
	auto* relStorage = world.GetComponentVector<RelationshipComponent>();

	for (size_t i = 0; i < tagStorage->reverse.size(); ++i) {
		Entity entity = tagStorage->reverse[i];
		json item;

		// NOWE: Zapisujemy stare, oryginalne ID tej encji
		item["id"] = entity.id;

		item["name"] = tagStorage->dense[i].Tag;

		if (transformStorage) {
			if (auto* transform = transformStorage->Get(entity)) {
				item["position"] = { transform->Position.x, transform->Position.y, transform->Position.z };
				item["rotation"] = { transform->Rotation.x, transform->Rotation.y, transform->Rotation.z };
				item["scale"] = { transform->Scale.x, transform->Scale.y, transform->Scale.z };
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
			auto* script = scriptStorage->Get(entity);
			// Sprawdzamy, czy skrypt istnieje i czy ma zapisan¹ nazwê
			if (script && script->InstantiateScript && !script->ScriptName.empty()) {
				// Zamiast sztywnego tekstu, u¿ywamy zmiennej!
				item["script"] = script->ScriptName;
			}
		}

		// NOWE: Zapisywanie informacji o rodzicu
		if (relStorage) {
			if (auto* rel = relStorage->Get(entity)) {
				if (rel->Parent != NULL_ENTITY) {
					item["parent_id"] = rel->Parent; // Zapisujemy tylko ID rodzica
				}
			}
		}

		data["entities"].push_back(item);
	}

	std::ofstream file(filepath);
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