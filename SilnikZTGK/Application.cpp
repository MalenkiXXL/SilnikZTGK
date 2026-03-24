#include "Application.h"
#include <iostream>

Application* Application::s_Instance = nullptr;

Application::Application()
{
	s_Instance = this;
	m_Window = new Window(800, 600, "Silnik");
	m_Window->Init();
	m_Window->SetEventCallback([this](Event& e) { OnEvent(e); }); //stworz niewidzialna funkcje ktora zna ten wskaznik i niech wywola OnEvent
}

Application::~Application()
{
	delete m_Window;
}

void Application::Run()
{

    // 6. G£ÓWNA PÊTLA GRY
	while (m_Running)
	{
		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_Window->OnUpdate();
	}
}

void Application::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return OnWindowClose(e); });
	dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return OnWindowResize(e); });
	dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) { return OnKeyPressed(e); });
}

bool Application::OnWindowClose(WindowCloseEvent& e)
{
	m_Running = false;
	std::cout << "Zamykam okno silnika! " << std::endl;
	return true;
}

bool Application::OnWindowResize(WindowResizeEvent& e)
{
	int width = e.GetWidth();
	int height = e.GetHeight();
	std::cout << "Zmieniam rozmiar okna " << width << " x " << height << std::endl;
	glViewport(0, 0, width, height);
	return true;
}

bool Application::OnKeyPressed(KeyPressedEvent& e)
{
	std::cout << "Wcisnieto klawisz: " << e.GetKeyCode() << std::endl;;
	return true;
}




