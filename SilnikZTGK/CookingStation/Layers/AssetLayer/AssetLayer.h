#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Renderer/Shader.h"
#include "AssetManager.h"
#include <vector>
#include <memory>
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Scene/SceneSerializer.h"
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Renderer/ShaderLibrary.h"

#include "CookingStation/Core/Timestep.h"

class AssetLayer : public Layer
{
	friend class SceneSerializer;

public:
	AssetLayer() : Layer("AssetLayer") {}
	~AssetLayer();
	void OnAttach();
	void OnUpdate(Timestep ts);
	void OnEvent(Event& e);
	bool OnWindowResize(WindowResizeEvent& e);
	void SetScene(std::shared_ptr<Scene> scene) { m_ActiveScene = scene; }

protected:
	std::shared_ptr<Scene> m_ActiveScene;

private:
	ShaderLibrary m_ShaderLibrary;
	std::shared_ptr<Shader> m_Shader;
	float m_ViewportWidth = 800.0f;
	float m_ViewportHeight = 600.0f;
};

