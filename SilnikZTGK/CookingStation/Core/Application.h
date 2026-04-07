#pragma once
#include "CookingStation/Events/Event.h"
#include "CookingStation/Events/KeyEvent.h"
#include "CookingStation/Events/MouseEvent.h"
#include "CookingStation/Events/WindowEvent.h"
#include "Layer.h"
#include "Window.h"
#include "CookingStation/Layers/CameraLayer/CameraLayer.h"
#include "CookingStation/Layers/AssetLayer/AssetLayer.h"
#include "CookingStation/Layers/GuiLayer/GuiLayer.h"
#include "CookingStation/Layers/EditorLayer/EditorLayer.h"

#include <vector>

class Application
{
public:
	Application();
	~Application();

	inline static Application& Get() { return *s_Instance; }
	inline Window& GetWindow() { return *m_Window; }


	void Run(); //Glowna petla while
	void OnEvent(Event& e); //Odbiornik Eventow

	void PushLayer(Layer* layer);
	std::vector<Layer*> m_LayerStack;

private:
	bool OnWindowClose(WindowCloseEvent& e);
	bool OnWindowResize(WindowResizeEvent& e);
	bool OnKeyPressed(KeyPressedEvent& e);

	Window* m_Window; //wskaznik na okno
	bool m_Running = true;

	float m_LastFrameTime = 0.0f;

	static Application* s_Instance;
};