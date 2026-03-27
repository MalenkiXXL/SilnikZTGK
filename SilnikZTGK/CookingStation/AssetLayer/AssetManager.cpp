#include "AssetManager.h"
#include <iostream>

std::map<std::string, std::shared_ptr<Model>> AssetManager::m_Models;

std::shared_ptr<Model> AssetManager::GetModel(const std::string& path) {
	auto it = m_Models.find(path);
	if (it != m_Models.end()) {
		return it->second;
	}

	std::cout << "[AssetManager] Laduje: " << path << std::endl;
	auto newModel = std::make_shared<Model>(path);
	m_Models[path] = newModel;
	return newModel;
}

void AssetManager::Clean() {
	m_Models.clear();
}

