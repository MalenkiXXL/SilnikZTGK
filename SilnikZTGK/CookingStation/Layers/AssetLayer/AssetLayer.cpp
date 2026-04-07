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

	// wczytujemy definicje modeli do biblioteki z pliku
    AssetManager::LoadModelLibrary("CookingStation/Assets/modelsLib.json");

	// inicjalizacja nowej sceny, jezeli nie zostala przekazana
	if (!m_ActiveScene) {
		m_ActiveScene = std::make_shared<Scene>(); 
	}

	// pobieramy dostêp do swiata ECS
	auto& world = m_ActiveScene->GetWorld(); 

	// rejestrujemy typy komponentow by przygotowac pamiec
	world.RegisterComponent<TagComponent>();
	world.RegisterComponent<MeshComponent>();
	world.RegisterComponent<TransformComponent>();

	// wczytujemy konkretne obiekty i ich stan z pliku zapisu
	SceneSerializer serializer(m_ActiveScene.get());
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

