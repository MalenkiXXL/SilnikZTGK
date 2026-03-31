#include "CameraLayer.h"
#include "../../Core/Input.h"
#include <iostream>
#include <GLFW/glfw3.h>
#include "CookingStation/Layers/GuiLayer/Gui.h"

CameraLayer::CameraLayer() : Layer("CameraLayer"), m_Camera(glm::vec3(0.f,0.f,10.f))
{};
CameraLayer::~CameraLayer() {};

void CameraLayer::OnUpdate() {

	if (Gui::AnyItemActive()) return; //sprawdza czy cokolwiek w Gui jest aktywne
	float deltaTime = 0.016f; // potem bedzie pobierane z klasy application
	if (Input::IsKeyPressed(GLFW_KEY_W)) {
		m_Camera.ProcessKeyboard(UP, deltaTime*0.1);
	}
	if (Input::IsKeyPressed(GLFW_KEY_S)) {
		m_Camera.ProcessKeyboard(DOWN, deltaTime*0.1);
	}
	if (Input::IsKeyPressed(GLFW_KEY_D)) {
		m_Camera.ProcessKeyboard(RIGHT, deltaTime*0.1);
	}
	if (Input::IsKeyPressed(GLFW_KEY_A)) {
		m_Camera.ProcessKeyboard(LEFT, deltaTime*0.1);
	}

	if (m_ActiveScene) {
		m_ActiveScene->SetCamera(&m_Camera);
	}
	//std::cout << "Position: x:" << m_Camera.Position.x << " y" << m_Camera.Position.y << " z:" << m_Camera.Position.z << std::endl;

};
