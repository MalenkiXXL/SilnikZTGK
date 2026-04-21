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

	// Pêtla tworz¹ca obiekty gry
	if (data.contains("entities")) {
		for (auto& item : data["entities"]) {

			// BEZPIECZNE wyci¹ganie informacji 
			std::string name = item.contains("name") ? item["name"].get<std::string>() : "Nowy Obiekt";
			std::string modelPath = item.contains("model_path") ? item["model_path"].get<std::string>() : "";

			// Tworzymy encjê w œwiecie
			auto builder = m_Scene->GetWorld().BuildEntity();

			// Zawsze dajemy jej nazwê
			builder.With<TagComponent>({ name });

			// 1. BEZPIECZNE ³adowanie modelu
			if (!modelPath.empty()) {
				auto model = AssetManager::GetModel(modelPath);

				// Tworzymy komponent i jawnie przypisujemy mu model
				MeshComponent meshComp;
				meshComp.ModelPtr = model;
				builder.With<MeshComponent>(meshComp);
			}

			// 2. KULOODPORNE ³adowanie transformacji (Niezale¿nie od kolejnoœci zmiennych w .h!)
			TransformComponent transComp;
			if (item.contains("position") && item.contains("rotation") && item.contains("scale")) {
				transComp.Position = { item["position"][0], item["position"][1], item["position"][2] };
				transComp.Rotation = { item["rotation"][0], item["rotation"][1], item["rotation"][2] };
				transComp.Scale = { item["scale"][0], item["scale"][1], item["scale"][2] };
			}
			else {
				transComp.Position = { 0.0f, 0.0f, 0.0f };
				transComp.Rotation = { 0.0f, 0.0f, 0.0f };
				transComp.Scale = { 1.0f, 1.0f, 1.0f }; // Upewniamy siê, ¿e domyœlna skala to 1, a nie 0!
			}
			builder.With<TransformComponent>(transComp);

			if (item.contains("collider")) {
				BoxColliderComponent bc;
				bc.Size = { item["collider"]["size"][0], item["collider"]["size"][1], item["collider"]["size"][2] };
				bc.Offset = { item["collider"]["offset"][0], item["collider"]["offset"][1], item["collider"]["offset"][2] };
				builder.With<BoxColliderComponent>(bc);
			}

			// Wczytywanie Skryptów
			if (item.contains("script")) {
				NativeScriptComponent nsc;
				std::string scriptName = item["script"].get<std::string>();

				
				if (scriptName == "RotationScript") {
					nsc.Bind<RotationScript>();
				}

				builder.With<NativeScriptComponent>(nsc);
			}

			builder.Build(); // Finalizacja
			spdlog::info("[SceneLoader] Dodano: {}", name);
		}
	}

	return true; // scena wczytana pomyslnie
}

void SceneSerializer::Serialize(const std::string& filepath) {
	json data;
	auto& world = m_Scene->GetWorld();

	data["entities"] = json::array();

	auto* tagStorage = world.GetComponentVector<TagComponent>();
	if (!tagStorage) return; // Jak nie ma tagów, nie ma czego zapisywaæ

	//Pobieramy kontenery pamiêci raz przed wejœciem do pêtli
	auto* transformStorage = world.GetComponentVector<TransformComponent>();
	auto* meshStorage = world.GetComponentVector<MeshComponent>();
	auto* colliderStorage = world.GetComponentVector<BoxColliderComponent>();
	auto* scriptStorage = world.GetComponentVector<NativeScriptComponent>();

	for (size_t i = 0; i < tagStorage->reverse.size(); ++i) {
		Entity entity = tagStorage->reverse[i];
		json item;

		// Tagi
		item["name"] = tagStorage->dense[i].Tag;

		// Transformacje 
		if (transformStorage) {
			if (auto* transform = transformStorage->Get(entity)) {
				item["position"] = { transform->Position.x, transform->Position.y, transform->Position.z };
				item["rotation"] = { transform->Rotation.x, transform->Rotation.y, transform->Rotation.z };
				item["scale"] = { transform->Scale.x, transform->Scale.y, transform->Scale.z };
			}
		}

		// Modele
		if (meshStorage) {
			auto* mesh = meshStorage->Get(entity); 
			if (mesh && mesh->ModelPtr) {
				item["model_path"] = mesh->ModelPtr->FilePath;
			}
		}
		// Collidery
		if (colliderStorage) {
			if (auto* collider = colliderStorage->Get(entity)) {
				item["collider"]["size"] = { collider->Size.x, collider->Size.y, collider->Size.z };
				item["collider"]["offset"] = { collider->Offset.x, collider->Offset.y, collider->Offset.z };
			}
		}

		// Skrypty
		if (scriptStorage) {
			auto* script = scriptStorage->Get(entity); 
			if (script && script->InstantiateScript) {
				item["script"] = "RotationScript";
			}
		}

		data["entities"].push_back(item);
	}

	std::ofstream file(filepath);
	if (file.is_open()) {
#ifdef _DEBUG // Standardowe makro Visual Studio dla trybu Debug
		// £adny, czytelny format dla Debug
		file << data.dump(4);
#else
		// b³yskawiczny odczyt/zapis w Release
		file << data.dump();
#endif
		// ----------------------

		spdlog::info("[SceneSerializer] Zapisano scene do: {}", filepath);
	}
	else {
		spdlog::error("[SceneSerializer] Nie udalo sie zapisac pliku {}", filepath);
	}
}