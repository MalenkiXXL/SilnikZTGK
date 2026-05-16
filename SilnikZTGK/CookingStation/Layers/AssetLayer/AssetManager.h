#pragma once
#include <map>
#include <string>
#include <memory>
#include <fstream>

#include "CookingStation/Renderer/Model.h"
#include "CookingStation/Renderer/Mesh.h"
#include "CookingStation/Core/Texture.h"
#include "CookingStation/Renderer/ShaderLibrary.h"
#include "CookingStation/Layers/AssetLayer/Animation.h"
#include "CookingStation/Renderer/Texture2D.h"

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
	static void InitCoreAssets();
	static ShaderLibrary& GetShaders() { return s_Shaders; }
	static std::shared_ptr<Texture> GetTexture(const std::string& path);
	static std::shared_ptr<Animation> LoadAnimation(const std::string& name, const std::string& path, Model* model);
	static std::shared_ptr<Animation> GetAnimation(const std::string& name);
	static std::shared_ptr<Texture2D> GetTexture2D(const std::string& path);

private:
	static std::unordered_map<std::string, std::shared_ptr<Model>> m_Models;
	static std::vector<ModelLibraryEntry> s_Library;
	static ShaderLibrary s_Shaders;
	static std::unordered_map<std::string, std::shared_ptr<Texture>> m_Textures;
	static std::unordered_map<std::string, std::shared_ptr<Animation>> s_Animations;
	static std::unordered_map<std::string, std::shared_ptr<Texture2D>> m_Textures2D;
};

