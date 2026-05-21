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
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Core/VFS/IFileSystem.h"
#include "CookingStation/Core/VFS/PhysicalFileSystem.h"
#include "CookingStation/Core/VFS/VFS.h"
#include "CookingStation/Layers/GuiLayer/Gui.h"
#include "CookingStation/Scripts/ScriptRegistry.h"
#include <iostream>

Application* Application::s_Instance = nullptr;

Application::Application()
{
	s_Instance = this;
	m_Window = new Window(800, 600, "Silnik");
	m_Window->Init();
	m_Window->SetEventCallback([this](Event& e) { OnEvent(e); });

	glEnable(GL_MULTISAMPLE);

	// 1. ZWYK£Y FBO (GUI)
	FramebufferSpecification fbSpec;
	fbSpec.Width = m_Window->GetWidth();
	fbSpec.Height = m_Window->GetHeight();
	fbSpec.Samples = 1;
	m_ViewportFBO = std::make_shared<Framebuffer>(fbSpec);

	// 2. FBO z MSAA (3D)
	FramebufferSpecification msaaSpec;
	msaaSpec.Width = m_Window->GetWidth();
	msaaSpec.Height = m_Window->GetHeight();
	msaaSpec.Samples = 4;
	m_MsaaFBO = std::make_shared<Framebuffer>(msaaSpec);

#ifdef CS_DISTRIBUTION
	std::string assetsPath = "CookingStation/Assets";
	std::string shadersPath = "CookingStation/Shaders";
#else
	std::string assetsPath = "CookingStation/Assets";
	std::string shadersPath = "CookingStation/Shaders";
#endif

	std::shared_ptr<PhysicalFileSystem> physicalFS = std::make_shared<PhysicalFileSystem>(assetsPath);
	VFS::Mount("assets", physicalFS);
	std::shared_ptr<PhysicalFileSystem> shaderFS = std::make_shared<PhysicalFileSystem>(shadersPath);
	VFS::Mount("shaders", shaderFS);

	SceneManager::NewScene();
	Renderer::Init();
	Renderer2D::Init();

#ifdef CS_DISTRIBUTION
	{
		AssetManager::LoadModelLibrary("assets://modelsLib.json");
		AssetManager::InitCoreAssets();
		ScriptRegistry::Init();   

		std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
		SceneSerializer serializer(activeScene.get());

		if (serializer.Deserialize("assets://levels/level01.json"))
		{
			activeScene->SetViewportSize(m_Window->GetWidth(), m_Window->GetHeight());

			// Gui musi znaæ rozmiar ekranu do poprawnego mapowania myszy
			Gui::SetScreenSize((float)m_Window->GetWidth(), (float)m_Window->GetHeight());

			activeScene->SetState(SceneState::Play);
			activeScene->OnRuntimeStart();
		}
		else
		{
			spdlog::error("Blad krytyczny: Nie udalo sie wczytac sceny: assets://levels/level01.json");
		}
	}
#endif

	PushLayer(new CameraLayer());
	PushLayer(new AssetLayer());
	PushLayer(new GameLayer());   

	auto renderLayer = new RendererLayer();
	renderLayer->SetTargetFramebuffer(m_MsaaFBO);
	renderLayer->SetResolveTarget(m_ViewportFBO);
	PushLayer(renderLayer);

	PushLayer(new HUDLayer());

#ifndef CS_DISTRIBUTION
	auto editorLayer = new EditorLayer();
	editorLayer->SetTargetFramebuffer(m_ViewportFBO);
	PushLayer(editorLayer);

	auto editorGuiLayer = new EditorGuiLayer();
	editorGuiLayer->SetViewportFramebuffer(m_ViewportFBO);
	editorGuiLayer->SetMsaaFramebuffer(m_MsaaFBO);
	PushLayer(editorGuiLayer);
#endif

	auto gameGuiLayer = new GameGuiLayer();
	gameGuiLayer->SetViewportFramebuffer(m_ViewportFBO);
	PushLayer(gameGuiLayer);

	AudioEngine::Init();
}

Application::~Application()
{
	delete m_Window;
	AudioEngine::Shutdown();
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

		m_MsaaFBO->ResolveTo(m_ViewportFBO);

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

	if (width == 0 || height == 0)
	{
		return false;
	}

	// Ta linijka wykonuje siê ZAWSZE (zarówno w Edytorze, jak i w gotowej grze)
	glViewport(0, 0, width, height);

#ifdef CS_DISTRIBUTION

	// 1. Dopasowujemy rozmiary buforów renderowania do pe³nego okna gry
	if (m_ViewportFBO) m_ViewportFBO->Resize(width, height);
	if (m_MsaaFBO) m_MsaaFBO->Resize(width, height);

	// 2. Bezpiecznie aktualizujemy wymiary ekranu dla silnika GUI przez nasz nowy Setter
	Gui::SetScreenSize((float)width, (float)height);

	// 3. Aktualizujemy Aspect Ratio kamery w œwiecie gry, ¿eby obiekty nie by³y sp³aszczone
	auto activeScene = SceneManager::GetActiveScene();
	if (activeScene)
	{
		activeScene->SetViewportSize(width, height);
	}
#endif

	return false;
}

bool Application::OnKeyPressed(KeyPressedEvent& e)
{
	//	std::cout << "Wcisnieto klawisz: " << e.GetKeyCode() << std::endl;;
	return false;
}




