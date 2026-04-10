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

			builder.Build(); // Finalizacja
			spdlog::info("[SceneLoader] Dodano: {}", name);
		}
	}

	return true; // scena wczytana pomyslnie
}


void SceneSerializer::Serialize(const std::string& filepath) {
	json data;

	// pobieramy wektory encji z ClearColorComponent
	auto* clearColorSet = m_Scene->GetWorld().GetComponentVector<ClearColorComponent>();
	// zak³adamy ¿e tylko jedna encja ma kolor    
	if (clearColorSet && !clearColorSet->dense.empty()) {
		auto& c = clearColorSet->dense[0].bgColor;
		// zapisujemy teraz r, g, b ORAZ a
		data["settings"]["clear_color"] = { c.r, c.g, c.b, c.a };
	}

	// inicjalizacja tablicy json na obiekty
	data["entities"] = json::array();

	auto* tagSet = m_Scene->GetWorld().GetComponentVector<TagComponent>();
	if (tagSet) {
		// reverse to zbior zawieraj¹cy wszystkie encje z dany komponentem
		for (size_t i = 0; i < tagSet->reverse.size(); ++i) {

			Entity entity = tagSet->reverse[i];
			json item;

			// zapisywanie nazwy 
			item["name"] = tagSet->dense[i].Tag;

			// zapisywanie transformacji
			auto* transformSet = m_Scene->GetWorld().GetComponent<TransformComponent>(entity);
			if (transformSet) {
				item["position"] = { transformSet->Position.x, transformSet->Position.y, transformSet->Position.z };
				item["rotation"] = { transformSet->Rotation.x, transformSet->Rotation.y, transformSet->Rotation.z };
				item["scale"] = { transformSet->Scale.x, transformSet->Scale.y, transformSet->Scale.z };
			};

			// Zapisywanie modelu, TYLKO jeœli encja go posiada
			auto* meshSet = m_Scene->GetWorld().GetComponent<MeshComponent>(entity);
			if (meshSet && meshSet->ModelPtr) {
				item["model_path"] = meshSet->ModelPtr->FilePath;
			};

			data["entities"].push_back(item);
		}
	}

	std::ofstream file(filepath);
	if (file.is_open()) {
		file << data.dump(4);
		spdlog::info("[SceneSerializer] Zapisano scene do: {}", filepath);
	}
	else {
		spdlog::error("[SceneSerializer] Nie udalo sie zapisac pliku {}", filepath);
	}
}