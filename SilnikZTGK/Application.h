#pragma once
#include "Events/Event.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "Events/WindowEvent.h"

#include "Window.h"

class Application
{
public:
	Application();
	~Application();

	void Run(); //Glowna petla while
	void OnEvent(Event& e); //Odbiornik Eventow

private:
	bool OnWindowClose(WindowCloseEvent& e);
	bool OnWindowResize(WindowResizeEvent& e);

	Window* m_Window; //wskaznik na okno
	bool m_Running = true;
};