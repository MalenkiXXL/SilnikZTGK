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
#include "CookingStation/Scripts/ScriptRegistry.h"
#include <nlohmann/json.hpp>
#include <glm/glm.hpp>


using json = nlohmann::json;

AssetLayer::~AssetLayer() {};
void AssetLayer::OnAttach()
{
    AssetManager::LoadModelLibrary("assets://modelsLib.json");
    AssetManager::InitCoreAssets();
    ScriptRegistry::Init();

    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    if (!activeScene) {
        spdlog::error("AssetLayer: Brak aktywnej sceny!");
        return;
    }

#ifndef CS_DISTRIBUTION
    // W trybie edytora deserializujemy tutaj normalnie
    auto& world = activeScene->GetWorld();
    SceneSerializer serializer(activeScene.get());
    serializer.Deserialize("assets://levels/level02.json");
#endif
}

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


