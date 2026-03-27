#include "AssetLayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

AssetLayer::~AssetLayer() {};
void AssetLayer::OnAttach() {
	glEnable(GL_DEPTH_TEST);

	m_Shader = std::make_unique<Shader>("CookingStation/Shaders/vsShaders/shader.vs", "CookingStation/Shaders/fragShaders/shader.frag");
	auto marchewaModel = AssetManager::GetModel("CookingStation/Assets/marchewa/marchewa.obj");

	/*Entity e1 = { "Marchewa_1", marchewaModel, glm::vec3(0,0,0), glm::vec3(1.0) };
	m_Entities.push_back(e1);
	std::cout << "Model " << e1.Name << ", pozycja: x: " << e1.Position.x << ", y: " << e1.Position.y << ", z:" << e1.Position.z;*/
	
	if (!m_ActiveScene) {
		m_ActiveScene = std::make_shared<Scene>();
	}

	auto& world = m_ActiveScene->GetWorld();

	world.RegisterComponent<TagComponent>();
	world.RegisterComponent<MeshComponent>();
	world.RegisterComponent<TransformComponent>();

	SceneSerializer serializer(m_ActiveScene.get());
	serializer.Deserialize("CookingStation/example.json");
};


void AssetLayer::OnUpdate() {
	float aspectRatio = m_ViewportWidth / (m_ViewportHeight > 0 ? m_ViewportHeight : 1.0f);

	m_Shader->use();

	if (m_ActiveScene && m_ActiveScene->GetCamera()) {
		glm::mat4 view = m_ActiveScene->GetCamera()->GetViewMatrix();
		m_Shader->setMat4("view", view);
		m_Shader->setVec3("viewPos", m_ActiveScene->GetCamera()->Position);
	}
	
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
	m_Shader->setMat4("projection", projection);

	m_Shader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_Shader->setVec3("lightPos", glm::vec3(5.0f, 5.0f, 10.0f));    

	if (m_ActiveScene) {

		auto& world = m_ActiveScene->GetWorld();

		auto* meshStorage = world.GetComponentVector<MeshComponent>();
		auto* transformStorage = world.GetComponentVector<TransformComponent>();
	
		for (size_t i = 0; i < meshStorage->dense.size(); i++) {
			auto& meshComp = meshStorage->dense[i];

			Entity owner = meshStorage->reverse[i];

			TransformComponent* transform = transformStorage->Get(owner);

			if (transform && meshComp.ModelPtr) {
				glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), transform->Position);
				modelMatrix = glm::scale(modelMatrix, transform->Scale);

				m_Shader->setMat4("model", modelMatrix);
				meshComp.ModelPtr->Draw(*m_Shader);
			}
		}
	}
};

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


