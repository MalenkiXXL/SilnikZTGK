#include "Application.h"
#include "Input.h"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <iostream>
#include "ProfileTimer.h"
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Renderer/RenderCommand.h"
#include "CookingStation/Layers/RenderLayer/RendererLayer.h"
#include "CookingStation/Layers/HudLayer/HUDLayer.h"
#include "CookingStation/Layers/MainMenuLayer/MainMenuLayer.h"
#include "CookingStation/Core/GraphicsSettings.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Core/Timestep.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Core/VFS/IFileSystem.h"
#include "CookingStation/Core/VFS/PhysicalFileSystem.h"
#include "CookingStation/Core/VFS/VFS.h"
#include "CookingStation/Core/VFS/PackageFileSystem.h"
#include "CookingStation/Layers/GuiLayer/Utils/Gui.h"
#include "CookingStation/Scripts/ScriptRegistry.h"
#include "CookingStation/Events/GamepadEvent.h"
#include "CookingStation/Events/GameEvents.h"

Application* Application::s_Instance = nullptr;

Application::Application()
{
	s_Instance = this;
	m_Window = new Window(800, 600, "Silnik");
	m_Window->Init();
	m_Window->SetEventCallback([this](Event& e) { OnEvent(e); });

	glEnable(GL_MULTISAMPLE);

	FramebufferSpecification fbSpec;
	fbSpec.Width = m_Window->GetWidth();
	fbSpec.Height = m_Window->GetHeight();
	fbSpec.Samples = 1;
	fbSpec.HDR = true;
	m_ViewportFBO = std::make_shared<Framebuffer>(fbSpec);

	FramebufferSpecification msaaSpec;
	msaaSpec.Width = m_Window->GetWidth();
	msaaSpec.Height = m_Window->GetHeight();
	msaaSpec.Samples = 4;
	msaaSpec.HDR = true;
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
	AudioEngine::Init();

	GetEventBus().Subscribe<AudioSettingsChangedEvent>([](const AudioSettingsChangedEvent& e) {
		AudioEngine::SetMusicEnabled(e.MusicEnabled);
		AudioEngine::SetSoundsEnabled(e.SoundsEnabled);
		spdlog::info("Zaktualizowano audio (Muzyka: {}, Dzwieki: {})", e.MusicEnabled, e.SoundsEnabled);
		});

#ifdef CS_DISTRIBUTION
	{
		AssetManager::LoadModelLibrary("assets://modelsLib.json");
		AssetManager::InitCoreAssets();
		ScriptRegistry::Init();

		std::shared_ptr<Scene> activeScene = SceneManager::NewScene();
		Gui::SetScreenSize((float)m_Window->GetWidth(), (float)m_Window->GetHeight());
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

#ifdef CS_DISTRIBUTION
	auto mainMenuLayer = new MainMenuLayer();
	PushLayer(mainMenuLayer);
#endif
}

Application::~Application()
{
	for (Layer* layer : m_LayerStack)
	{
		layer->OnDetach();
		delete layer;
	}
	m_LayerStack.clear();

	m_ViewportFBO.reset();
	m_MsaaFBO.reset();
	Renderer2D::Shutdown();
	Renderer::Shutdown();
	Gui::Shutdown();         
	SceneManager::Shutdown();
	AudioEngine::Shutdown();


	AssetManager::Clean();
	
	glFinish();
	delete m_Window;
	glfwTerminate();

	spdlog::shutdown();
}

void Application::ApplyGraphicsSettings()
{
	auto& settings = GraphicsSettings::Get();
	int width = settings.WindowWidth;
	int height = settings.WindowHeight;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// TA LINIJKA WYWOŁYWAŁA BŁĄD, JEŚLI ZOSTAŁA POMINIĘTA PRZY KOPIOWANIU!
	GLFWwindow* nativeWindow = (GLFWwindow*)m_Window->GetNativeWindow();

	if (settings.Fullscreen) {
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		glfwSetWindowMonitor(nativeWindow, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		width = mode->width;
		height = mode->height;
	}
	else {
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		int xpos = (mode->width - width) / 2;
		int ypos = (mode->height - height) / 2;
		glfwSetWindowMonitor(nativeWindow, nullptr, xpos, ypos, width, height, 0);
	}

	glViewport(0, 0, width, height);

	if (m_MsaaFBO)
	{
		m_MsaaFBO->SetSamples(std::max(1, settings.MsaaSamples));
		m_MsaaFBO->Resize(width, height);
	}

	if (m_ViewportFBO)
	{
		m_ViewportFBO->Resize(width, height);
	}

	Gui::SetScreenSize((float)width, (float)height);

	auto activeScene = SceneManager::GetActiveScene();
	if (activeScene)
	{
		activeScene->SetViewportSize(width, height);
	}

	spdlog::info("GraphicsSettings: Zastosowano MSAA x{} @ {}x{} (Fullscreen: {})",
		settings.MsaaSamples, width, height, settings.Fullscreen);
}

void Application::Run()
{
	while (m_Running)
	{
		float time = (float)glfwGetTime();
		Timestep timestep = time - m_LastFrameTime;
		m_LastFrameTime = time;

		if (timestep > 0.1f)
			timestep = 0.1f;

		RenderCommand::SetClearColor(glm::vec4(0.05f, 0.05f, 0.05f, 1.0f));
		RenderCommand::Clear();

		Renderer::ResetStats();

		{
			ProfileTimer timer(Renderer::GetStats().CPULogicTime);

			for (Layer* layer : m_LayerStack)
			{
				layer->OnUpdate(timestep);
			}
		}

		m_Window->OnUpdate();

		Input::Update();
	}
}

void Application::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);

	dispatcher.Dispatch<GamepadButtonPressedEvent>([](GamepadButtonPressedEvent& event) {
		return false;
		});

	dispatcher.Dispatch<GamepadAxisMovedEvent>([](GamepadAxisMovedEvent& event) {
		return false;
		});

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

	glViewport(0, 0, width, height);

#ifdef CS_DISTRIBUTION
	if (m_ViewportFBO) m_ViewportFBO->Resize(width, height);
	if (m_MsaaFBO) m_MsaaFBO->Resize(width, height);

	Gui::SetScreenSize((float)width, (float)height);

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
	return false;
}