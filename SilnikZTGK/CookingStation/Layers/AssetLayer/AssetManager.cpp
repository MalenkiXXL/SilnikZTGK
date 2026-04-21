#include "AssetManager.h"
#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

std::vector<ModelLibraryEntry> AssetManager::s_Library;
ShaderLibrary AssetManager::s_Shaders;

void AssetManager::LoadModelLibrary(const std::string& path){
	std::ifstream file(path);
	if (!file.is_open()) return;
	
	nlohmann::json data = nlohmann::json::parse(file);
	s_Library.clear();
	for (auto& item : data["models"]) {
		s_Library.push_back({ item["name"], item["path"] });
	}
}

std::unordered_map<std::string, std::shared_ptr<Model>> AssetManager::m_Models;

std::shared_ptr<Model> AssetManager::GetModel(const std::string& path) {
	auto it = m_Models.find(path);
	if (it != m_Models.end()) {
		return it->second;
	}

	spdlog::info("[AssetManager] Laduje: {}", path);

	auto newModel = std::make_shared<Model>(path);
	m_Models[path] = newModel;
	return newModel;
}

void AssetManager::Clean() {
	m_Models.clear();
}

void AssetManager::InitCoreAssets() {
	s_Shaders.Load("ModelShader",
		"CookingStation/Shaders/vsShaders/shader.vs",
		"CookingStation/Shaders/fragShaders/shader.frag");

	spdlog::info("[AssetManager] Zaladowano bazowe shadery.");
}
