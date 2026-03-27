#pragma once
#include <map>
#include <string>
#include <memory>
#include <fstream>

#include <../SilnikZTGK/CookingStation/Model.h>
#include <../SilnikZTGK/CookingStation/Mesh.h>

class AssetManager
{
public:
	static std::shared_ptr<Model> GetModel(const std::string& path);

	static void Clean();

private:
	static std::map<std::string, std::shared_ptr<Model>> m_Models;
};

