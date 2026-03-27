#pragma once
#include <../SilnikZTGK/CookingStation/Layer.h>
#include <../SilnikZTGK/CookingStation/Shader.h>
#include "AssetManager.h"
#include <vector>
#include <memory>
#include "../Events/WindowEvent.h"

//struktura pomocnicza dla obiektu na scenie
struct Entity {
	std::string Name;
	std::shared_ptr<Model> ModelPtr;
	glm::vec3 Position;
	glm::vec3 Scale;
};

class AssetLayer : public Layer
{
public:
	AssetLayer() : Layer("AssetLayer") {}
	~AssetLayer();
	void OnAttach();
	void OnUpdate();
	void OnEvent(Event& e);
	bool OnWindowResize(WindowResizeEvent& e);
private:
	std::unique_ptr<Shader> m_Shader;
	std::vector<Entity> m_Entities; //lista obiektów w warstwie
	float m_ViewportWidth = 800.0f;
	float m_ViewportHeight = 600.0f;
};

