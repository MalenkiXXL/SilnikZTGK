#include "AssetLayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

AssetLayer::~AssetLayer() {};
void AssetLayer::OnAttach() {
	glEnable(GL_DEPTH_TEST);

	m_Shader = std::make_unique<Shader>("CookingStation/Shaders/vsShaders/shader.vs", "CookingStation/Shaders/fragShaders/shader.frag");
	auto marchewaModel = AssetManager::GetModel("CookingStation/Assets/marchewa/marchewa.obj");

	Entity e1 = { "Marchewa_1", marchewaModel, glm::vec3(0,0,0), glm::vec3(1.0) };

	m_Entities.push_back(e1);

	std::cout << "Model " << e1.Name << ", pozycja: x: " << e1.Position.x << ", y: " << e1.Position.y << ", z:" << e1.Position.z;
};
void AssetLayer::OnUpdate() {
	float aspectRatio = m_ViewportWidth / (m_ViewportHeight > 0 ? m_ViewportHeight : 1.0f);

	m_Shader->use();
	
	m_Shader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_Shader->setVec3("lightPos", glm::vec3(5.0f, 5.0f, 10.0f));   
	m_Shader->setVec3("viewPos", glm::vec3(0.0f, -2.0f, -10.0f)); 

	glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
	glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, -10.0f));

	m_Shader->setMat4("projection", projection);
	m_Shader->setMat4("view", view);

	for (auto& entity : m_Entities)
	{
		glm::mat4 model = glm::translate(glm::mat4(1.0f), entity.Position);
		model = glm::scale(model, entity.Scale);

		m_Shader->setMat4("model", model);

		entity.ModelPtr->Draw(*m_Shader);
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


