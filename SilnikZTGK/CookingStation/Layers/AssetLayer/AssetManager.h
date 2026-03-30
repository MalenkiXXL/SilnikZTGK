#pragma once
#include <map>
#include <string>
#include <memory>
#include <fstream>

#include "CookingStation/Renderer/Model.h"
#include "CookingStation/Renderer/Mesh.h"
#include "CookingStation/Core/Texture.h"

struct ModelLibraryEntry {
	std::string Name;
	std::string Path;
};

class AssetManager
{
public:
	static void LoadModelLibrary(const std::string& path);
	static const std::vector<ModelLibraryEntry>& GetLibrary() { return s_Library; };
	static std::shared_ptr<Model> GetModel(const std::string& path);
	static void Clean();

private:
	static std::map<std::string, std::shared_ptr<Model>> m_Models;
	static std::vector<ModelLibraryEntry> s_Library;
};	

