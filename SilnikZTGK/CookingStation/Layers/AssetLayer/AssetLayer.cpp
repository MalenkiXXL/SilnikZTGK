#include "AssetLayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include "CookingStation/Renderer/Renderer.h"
#include <nlohmann/json.hpp>


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


void AssetLayer::OnUpdate() {
    auto& world = m_ActiveScene->GetWorld();
    auto* colorStorage = world.GetComponentVector<ClearColorComponent>();
    auto* meshStorage = world.GetComponentVector<MeshComponent>();
    auto* transformStorage = world.GetComponentVector<TransformComponent>();

    glm::vec4 clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
    if (colorStorage && !colorStorage->dense.empty()) {
        clearColor = colorStorage->dense[0].bgColor;
    }

    RenderCommand::SetClearColor(clearColor);
    RenderCommand::Clear();
	if (meshStorage) {
		for (size_t i = 0; i < meshStorage->dense.size(); i++) {
			Entity owner = meshStorage->reverse[i];
			TransformComponent* transform = transformStorage->Get(owner);
			auto& meshComp = meshStorage->dense[i];

			if (transform && meshComp.ModelPtr) {
				// Przekazujemy tylko model i transformacjê do renderera
				Renderer::Submit(m_Shader, meshComp.ModelPtr, transform->GetTransformMatrix());
			}
		}

	}
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

