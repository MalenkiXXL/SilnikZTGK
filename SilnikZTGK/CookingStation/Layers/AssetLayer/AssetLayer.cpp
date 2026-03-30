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

	m_Shader = std::make_unique<Shader>("CookingStation/Shaders/vsShaders/shader.vs", "CookingStation/Shaders/fragShaders/shader.frag");
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
    // 1. Przygotowanie danych i czyszczenie ekranu
    float aspectRatio = m_ViewportWidth / (m_ViewportHeight > 0 ? m_ViewportHeight : 1.0f);
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

    // 2. Renderowanie Œwiata 3D
    if (m_ActiveScene && m_ActiveScene->GetCamera()) {
        glm::mat4 view = m_ActiveScene->GetCamera()->GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
        glm::mat4 viewProjection = projection * view;

        Renderer::BeginScene(viewProjection);

        m_Shader->use();
        m_Shader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        m_Shader->setVec3("lightPos", glm::vec3(5.0f, 5.0f, 10.0f));
        m_Shader->setVec3("viewPos", m_ActiveScene->GetCamera()->Position);

        if (meshStorage) {
            for (size_t i = 0; i < meshStorage->dense.size(); i++) {
                auto& meshComp = meshStorage->dense[i];
                Entity owner = meshStorage->reverse[i];
                TransformComponent* transform = transformStorage->Get(owner);

                if (transform && meshComp.ModelPtr) {
                    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), transform->Position);
                    modelMatrix = glm::rotate(modelMatrix, glm::radians(transform->Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                    modelMatrix = glm::rotate(modelMatrix, glm::radians(transform->Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                    modelMatrix = glm::rotate(modelMatrix, glm::radians(transform->Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                    modelMatrix = glm::scale(modelMatrix, transform->Scale);

                    m_Shader->setMat4("viewProjection", viewProjection);
                    m_Shader->setMat4("model", modelMatrix);
                    meshComp.ModelPtr->Draw(*m_Shader);
                }
            }
        }
        Renderer::EndScene();
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

