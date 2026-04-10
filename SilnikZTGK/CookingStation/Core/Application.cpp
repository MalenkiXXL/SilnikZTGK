#include "Application.h"
#include "Input.h"
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Renderer/RenderCommand.h"
#include "CookingStation/Layers/RenderLayer/RendererLayer.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Core/Timestep.h"
#include <iostream>

Application* Application::s_Instance = nullptr;

Application::Application()
{
	s_Instance = this;
	m_Window = new Window(800, 600, "Silnik");
	m_Window->Init();
	m_Window->SetEventCallback([this](Event& e) { OnEvent(e); }); //stworz niewidzialna funkcje ktora zna ten wskaznik i niech wywola OnEvent

	// TWORZYMY PIERWSZ¥ SCENÊ PRZEZ MENED¯ERA SCEN
	SceneManager::NewScene();

	// DODAJEMY WARSTWY DO STOSU
	PushLayer(new CameraLayer());
	PushLayer(new AssetLayer());
	PushLayer(new RendererLayer());
	PushLayer(new GuiLayer());
	PushLayer(new EditorLayer());
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
	
		float time = (float)glfwGetTime();
		Timestep timestep = time - m_LastFrameTime;
		m_LastFrameTime = time;

		RenderCommand::SetClearColor(glm::vec4(.05f, 0.05f, 0.05f, 1.0f));
		RenderCommand::Clear();

		
		for (Layer* layer : m_LayerStack)
		{
			layer->OnUpdate(timestep);
		}

	/*	if (Input::IsKeyPressed(GLFW_KEY_W))
		{
			std::cout << "Wciskasz W!" << std::endl;
		}*/
		
		m_Window->OnUpdate();
		Input::Update();
	}
}

void Application::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return OnWindowClose(e); });
	dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return OnWindowResize(e); });
	dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) { return OnKeyPressed(e); });

	for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
	{
		if (e.Handled)
		{
			break;
		}
		(*it)->OnEvent(e);
	}
}

void Application::PushLayer(Layer* layer)
{
	m_LayerStack.push_back(layer);
	layer->OnAttach();
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
	return false;
}

bool Application::OnKeyPressed(KeyPressedEvent& e)
{
//	std::cout << "Wcisnieto klawisz: " << e.GetKeyCode() << std::endl;;
	return false;
}




