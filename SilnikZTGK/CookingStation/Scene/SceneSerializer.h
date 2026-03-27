#pragma once
#include <string>

class Scene;

class SceneSerializer {
public:
	SceneSerializer(Scene* scene) : m_Scene(scene) {};
	bool Deserialize(const std::string& filepath);
	void Serialize(const std::string& filepath);

private:
	Scene* m_Scene;
};