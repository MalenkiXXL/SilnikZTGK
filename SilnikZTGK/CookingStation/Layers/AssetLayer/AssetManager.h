#pragma once
#include <map>
#include <string>
#include <memory>
#include <fstream>

#include "CookingStation/Renderer/Model.h"
#include <CookingStation/Renderer/Mesh.h>

class AssetManager
{
public:
	static std::shared_ptr<Model> GetModel(const std::string& path);

	static void Clean();

private:
	static std::map<std::string, std::shared_ptr<Model>> m_Models;
};

