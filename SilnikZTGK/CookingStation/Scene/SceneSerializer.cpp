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
            .With<MeshComponent>({ model, modelPath }) // dodajemy mesh (wizualny model 3d)
   

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


void SceneSerializer::Serialize(const std::string& filepath) {
    json data;

    // pobieramy wektory encji z ClearColorComponent
    auto* clearColorSet = m_Scene->GetWorld().GetComponentVector<ClearColorComponent>();
    // zak³adamy ¿e tylko jedna encja ma kolor    
    if (clearColorSet && !clearColorSet->dense.empty()) {
        auto& c = clearColorSet->dense[0].bgColor;
        data["settings"]["clear_color"] = { c.r, c.g, c.b };
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
                item["position"] = { transformSet->Position.x,
                    transformSet->Position.y,
                    transformSet->Position.z
                };

                item["rotation"] = { transformSet->Rotation.x,
                    transformSet->Rotation.y,
                    transformSet->Rotation.z
                };

                item["scale"] = { transformSet->Scale.x,
                   transformSet->Scale.y,
                   transformSet->Scale.z
                };
            };

            auto* meshSet = m_Scene->GetWorld().GetComponent<MeshComponent>(entity);
            if (meshSet) {
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