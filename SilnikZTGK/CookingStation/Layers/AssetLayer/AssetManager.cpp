#include "AssetManager.h"
#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"
#include "CookingStation/Core/VFS/VFS.h"
#include "spdlog/spdlog.h"
#include "CookingStation/Layers/AssetLayer/Animation.h"

std::vector<ModelLibraryEntry> AssetManager::s_Library;
ShaderLibrary AssetManager::s_Shaders;

void AssetManager::LoadModelLibrary(const std::string& path) {
	// ZMIANA VFS
	std::vector<uint8_t> fileData = VFS::ReadFile(path);
	if (fileData.empty()) {
		spdlog::error("[AssetManager] Nie znaleziono pliku biblioteki modeli VFS: {}", path);
		return;
	}

	try {
		nlohmann::json data = nlohmann::json::parse(fileData.begin(), fileData.end());
		s_Library.clear();
		for (auto& item : data["models"]) {
			s_Library.push_back({ item["name"], item["path"] });
		}
	}
	catch (nlohmann::json::parse_error& e) {
		spdlog::error("[AssetManager] Blad JSON przy ladowaniu modeli: {}", e.what());
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
	m_Textures2D.clear();
}

void AssetManager::InitCoreAssets() {
	s_Shaders.Load("ModelShader",
		"shaders://vsShaders/shader.vs",
		"shaders://fragShaders/shader.frag");

	spdlog::info("[AssetManager] Zaladowano bazowe shadery z systemu VFS.");
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

std::unordered_map<std::string, std::shared_ptr<Texture2D>> AssetManager::m_Textures2D;

std::shared_ptr<Texture2D> AssetManager::GetTexture2D(const std::string& path) {
    // 1. Sprawdzamy, czy tekstura jest ju¿ w pamiêci RAM/VRAM
    auto it = m_Textures2D.find(path);
    if (it != m_Textures2D.end()) {
        return it->second; // Zwracamy gotowy wskaŸnik - zero wczytywania!
    }

    // 2. Jeœli jej nie ma, wczytujemy z VFS i zapisujemy do pamiêci na przysz³oœæ
    auto tex = std::make_shared<Texture2D>(path);
    m_Textures2D[path] = tex;
    return tex;
}