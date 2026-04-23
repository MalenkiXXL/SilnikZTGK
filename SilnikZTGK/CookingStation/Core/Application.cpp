#include "Application.h"
#include "Input.h"
#include <GLFW/glfw3.h>
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Renderer/RenderCommand.h"
#include "ProfileTimer.h"
#include "CookingStation/Layers/RenderLayer/RendererLayer.h"
#include "CookingStation/Layers/HudLayer/HUDLayer.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Core/Timestep.h"
#include <iostream>

Application* Application::s_Instance = nullptr;

Application::Application()
{
	s_Instance = this;
	m_Window = new Window(800, 600, "Silnik");
	m_Window->Init();
	m_Window->SetEventCallback([this](Event& e) { OnEvent(e); }); 

	FramebufferSpecification fbSpec;
	fbSpec.Width = m_Window->GetWidth();
	fbSpec.Height = m_Window->GetHeight();
	m_ViewportFBO = std::make_shared<Framebuffer>(fbSpec);
	SceneManager::NewScene();

	// DODAJEMY WARSTWY DO STOSU
	PushLayer(new CameraLayer());
	PushLayer(new AssetLayer());
	auto renderLayer = new RendererLayer();
	renderLayer->SetTargetFramebuffer(m_ViewportFBO);
	PushLayer(renderLayer);
	auto editorLayer = new EditorLayer();
	editorLayer->SetTargetFramebuffer(m_ViewportFBO);
	PushLayer(editorLayer);
	auto hudLayer = new HUDLayer();
	PushLayer(hudLayer);
	auto guiLayer = new GuiLayer();
	guiLayer->SetViewportFramebuffer(m_ViewportFBO);
	PushLayer(guiLayer);
}

Application::~Application()
{
	delete m_Window;
}

void Application::Run()
{
	// ==========================================
	// 6. G£ÓWNA PÊTLA GRY
	// ==========================================
	while (m_Running)
	{
		// 1. OBLICZANIE CZASU
		float time = (float)glfwGetTime();
		Timestep timestep = time - m_LastFrameTime;
		m_LastFrameTime = time;

		// Limitujemy maksymalny czas klatki.
		// Jeœli gra zatnie siê na chwile fizyka nie "eksploduje" wielkim skokiem.
		if (timestep > 0.1f)
			timestep = 0.1f;

		// 2. PRZYGOTOWANIE RENDERERA
		RenderCommand::SetClearColor(glm::vec4(0.05f, 0.05f, 0.05f, 1.0f));
		RenderCommand::Clear();

		// Czyœcimy liczniki przed rozpoczêciem klatki!
		Renderer::ResetStats();

		// 3. UPDATE LOGIKI I ECS
		{
			// timer startuje w tym momencie (konstruktor)
			ProfileTimer timer(Renderer::GetStats().CPULogicTime);

			for (Layer* layer : m_LayerStack)
			{
				layer->OnUpdate(timestep);
			}

			// gdy wychodzimy z klamerek { }, timer jest niszczony (destruktor).
			// oblicza miniony czas i wpisuje go do zmiennej CPULogicTime
		}

		// 4. AKTUALIZACJA OKNA I WEJŒCIA
		m_Window->OnUpdate(); // To wywo³a m.in. glfwSwapBuffers i glfwPollEvents

		Input::Update(); // Czyszczenie stanów wejœcia 
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




