#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "Entity.h"
#include "CookingStation/Renderer/Model.h"

class Scene;

class SceneSerializer {
public:
	SceneSerializer(Scene* scene) : m_Scene(scene) {};
	bool Deserialize(const std::string& filepath);
	void Serialize(const std::string& filepath);

    static bool ParseAnimatorFromJson(const nlohmann::json& item, std::shared_ptr<Model> model, AnimatorComponent& outAnimComp);

private:
	Scene* m_Scene;
};