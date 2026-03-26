#pragma once
#include "Events/Event.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "Events/WindowEvent.h"
#include "Layer.h"
#include "Window.h"
#include "CameraLayer/CameraLayer.h"

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

	static Application* s_Instance;
};