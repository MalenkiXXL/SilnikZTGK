#include "AssetLayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include "CookingStation/Scene/SceneSerializer.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Renderer/Renderer.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Core/Input.h"
#include <nlohmann/json.hpp>
#include <glm/glm.hpp>


using json = nlohmann::json;

AssetLayer::~AssetLayer() {};
void AssetLayer::OnAttach() {

	// wczytujemy definicje modeli do biblioteki z pliku
    AssetManager::LoadModelLibrary("CookingStation/Assets/modelsLib.json");

	// 2. Pobieramy aktualn¹ scenê utworzon¹ w Application.cpp przez SceneManagera
	std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();

	// Zabezpieczenie: jeœli z jakiegoœ powodu nie ma sceny, to przerywamy
	if (!activeScene) {
		spdlog::error("AssetLayer: Brak aktywnej sceny!");
		return;
	}

	// pobieramy dostêp do swiata ECS
	auto& world = activeScene->GetWorld(); 

	// wczytujemy konkretne obiekty i ich stan z pliku zapisu
	SceneSerializer serializer(activeScene.get());
	serializer.Deserialize("CookingStation/Assets/example.json");
};

void AssetLayer::OnUpdate(Timestep ts)
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

