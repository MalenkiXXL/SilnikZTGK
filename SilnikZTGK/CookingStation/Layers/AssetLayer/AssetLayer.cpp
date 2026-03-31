#include "AssetLayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include "CookingStation/Renderer/Renderer.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Core/Input.h"
#include <nlohmann/json.hpp>
#include <glm/glm.hpp>


using json = nlohmann::json;

AssetLayer::~AssetLayer() {};
void AssetLayer::OnAttach() {
    AssetManager::LoadModelLibrary("CookingStation/modelsLib.json");
	glEnable(GL_DEPTH_TEST);

	m_Shader = m_ShaderLibrary.Load("Standardowy", "CookingStation/Shaders/vsShaders/shader.vs", "CookingStation/Shaders/fragShaders/shader.frag");
	auto marchewaModel = AssetManager::GetModel("CookingStation/Assets/marchewa/marchewa.obj");
	
	if (!m_ActiveScene) {
		m_ActiveScene = std::make_shared<Scene>();
	}

	auto& world = m_ActiveScene->GetWorld();

	world.RegisterComponent<TagComponent>();
	world.RegisterComponent<MeshComponent>();
	world.RegisterComponent<TransformComponent>();

	SceneSerializer serializer(m_ActiveScene.get());
	serializer.Deserialize("CookingStation/example.json");
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
};


void AssetLayer::OnUpdate()
{

}

void AssetLayer::OnEvent(Event& e) {
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
		return OnWindowResize(ev);
		});
};

bool AssetLayer::OnWindowResize(WindowResizeEvent& e) {
	m_ViewportWidth = (float)e.GetWidth();
	m_ViewportHeight = (float)e.GetHeight();

	return false;
}

