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

bool SceneSerializer::Deserialize(const std::string& path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cerr << "Nie udalo sie otworzyc pliku " << path << std::endl;
		return false;
	}

	json data = json::parse(file);

    if (data.contains("settings")) {
        auto& settings = data["settings"];
        if (settings.contains("clear_color")) {
            auto& c = settings["clear_color"];
            m_Scene->GetWorld().BuildEntity()
                .With<ClearColorComponent>({ { c[0], c[1], c[2], c[3] } })
                .Build();
        }
    }

    for (auto& item : data["entities"]) {
        std::string name = item["name"].get<std::string>();
        std::string modelPath = item["model_path"].get<std::string>();
        auto model = AssetManager::GetModel(modelPath);

        // Budujemy encjê w œwiecie ECS
        m_Scene->GetWorld().BuildEntity()
            .With<TagComponent>({ name })
            .With<MeshComponent>({ model })
            .With<TransformComponent>({
                { item["position"][0], item["position"][1], item["position"][2] }, // Pozycja
                { item["rotation"][0], item["rotation"][1], item["rotation"][2]},  // Rotacja
                { item["scale"][0], item["scale"][1], item["scale"][2] }          // Skala
                })
            .Build();

        std::cout << "[SceneLoader] Dodano: " << name << std::endl;
    }

	return true;
}
