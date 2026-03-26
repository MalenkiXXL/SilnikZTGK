#include "CameraLayer.h"
#include "../Input.h"
#include <iostream>
#include <GLFW/glfw3.h>

CameraLayer::CameraLayer() : Layer("CameraLayer"), m_Camera(glm::vec3(0.f,0.f,0.f))
{};
CameraLayer::~CameraLayer() {};

void CameraLayer::OnUpdate() {
	float deltaTime = 0.016f; // potem bedzie pobierane z klasy application
	if (Input::IsKeyPressed(GLFW_KEY_W)) {
		m_Camera.ProcessKeyboard(FORWARD, deltaTime);
	}
	if (Input::IsKeyPressed(GLFW_KEY_S)) {
		m_Camera.ProcessKeyboard(BACKWARD, deltaTime);
	}
	if (Input::IsKeyPressed(GLFW_KEY_D)) {
		m_Camera.ProcessKeyboard(RIGHT, deltaTime);
	}
	if (Input::IsKeyPressed(GLFW_KEY_A)) {
		m_Camera.ProcessKeyboard(LEFT, deltaTime);
	}

	std::cout << "Position: x:" << m_Camera.Position.x << " y" << m_Camera.Position.y << " z:" << m_Camera.Position.z << std::endl;

};
