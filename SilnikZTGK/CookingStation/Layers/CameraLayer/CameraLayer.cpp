#include "CameraLayer.h"
#include "../../Core/Input.h"
#include <iostream>
#include <GLFW/glfw3.h>
#include "CookingStation/Layers/GuiLayer/Gui.h"
#include "CookingStation/Core/Timestep.h"
#include "CookingStation/Scene/SceneSerializer.h"
#include "CookingStation/Scene/SceneManager.h"

CameraLayer::CameraLayer() : Layer("CameraLayer"), m_Camera(glm::vec3(0.f,0.f,10.f))
{};
CameraLayer::~CameraLayer() {};

void CameraLayer::OnUpdate(Timestep ts) {

	std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
	if (activeScene) {
		activeScene->SetCamera(&m_Camera);
	}

	if (Gui::AnyItemActive()) return; //sprawdza czy cokolwiek w Gui jest aktywne
	if (Input::IsKeyPressed(GLFW_KEY_W)) {
		m_Camera.ProcessKeyboard(UP, ts*0.1);
	}
	if (Input::IsKeyPressed(GLFW_KEY_S)) {
		m_Camera.ProcessKeyboard(DOWN, ts*0.1);
	}
	if (Input::IsKeyPressed(GLFW_KEY_D)) {
		m_Camera.ProcessKeyboard(RIGHT, ts*0.1);
	}
	if (Input::IsKeyPressed(GLFW_KEY_A)) {
		m_Camera.ProcessKeyboard(LEFT, ts*0.1);
	}
};
