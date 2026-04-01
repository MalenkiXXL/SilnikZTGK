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

            // encja techniczna, przechowujaca kolor czyszczenia 
            m_Scene->GetWorld().BuildEntity()
                .With<ClearColorComponent>({ { c[0], c[1], c[2], c[3] } })
                .Build();
        }
    }

    // Pêtla tworz¹ca obiekty gry
    for (auto& item : data["entities"]) {
        // wyci¹gamy podstawowe informacje 
        std::string name = item["name"].get<std::string>();
        std::string modelPath = item["model_path"].get<std::string>();

        //  korzystamy z AssetManager aby sprawdziæ, czy model by³ juz za³adowany
        auto model = AssetManager::GetModel(modelPath);

        // budujemy encjê w œwiecie ECS
        m_Scene->GetWorld().BuildEntity()
            .With<TagComponent>({ name }) // dodajemy tag (nazwa w gui)
            .With<MeshComponent>({ model }) // dodajemy mesh (wizualny model 3d)

            // dodajemy transform z pliku json
            .With<TransformComponent>({
                { item["position"][0], item["position"][1], item["position"][2] },
                { item["rotation"][0], item["rotation"][1], item["rotation"][2]}, 
                { item["scale"][0], item["scale"][1], item["scale"][2] }     
                })
            .Build(); // finalizacja i zapisanie w encji w world 

        spdlog::info("[SceneLoader] Dodano:{}", name);
    }

	return true; //scena wczytana pomyslnie
}
