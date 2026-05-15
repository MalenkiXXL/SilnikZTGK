#include "AssetManager.h"
#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"
#include "CookingStation/Layers/AssetLayer/Animation.h"

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

	//spdlog::info("[AssetManager] Laduje: {}", path);

	auto newModel = std::make_shared<Model>(path);
	m_Models[path] = newModel;
	return newModel;
}

std::unordered_map<std::string, std::shared_ptr<Texture>> AssetManager::m_Textures;

std::shared_ptr<Texture> AssetManager::GetTexture(const std::string& path) {
	auto it = m_Textures.find(path);
	if (it != m_Textures.end())
		return it->second;

	auto tex = std::make_shared<Texture>(path);
	m_Textures[path] = tex;
	return tex;
}

void AssetManager::Clean() {
	m_Models.clear();
	m_Textures.clear();
}

void AssetManager::InitCoreAssets() {
	s_Shaders.Load("ModelShader",
		"CookingStation/Shaders/vsShaders/shader.vs",
		"CookingStation/Shaders/fragShaders/shader.frag");

	spdlog::info("[AssetManager] Zaladowano bazowe shadery.");
}

std::unordered_map<std::string, std::shared_ptr<Animation>> AssetManager::s_Animations;

std::shared_ptr<Animation> AssetManager::LoadAnimation(const std::string& name, const std::string& path, Model* model) {
	if (s_Animations.find(name) != s_Animations.end()) {
		return s_Animations[name]; // Zwróæ z pamiêci podrêcznej
	}

	auto animation = std::make_shared<Animation>(path, model);
	s_Animations[name] = animation;
	return animation;
}

std::shared_ptr<Animation> AssetManager::GetAnimation(const std::string& name) {
	if (s_Animations.find(name) != s_Animations.end())
		return s_Animations[name];
	return nullptr;
}