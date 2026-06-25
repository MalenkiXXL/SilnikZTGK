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

	Window::EnsureGLFWInitialized();

	auto& gs = GraphicsSettings::Get();
	int startWidth = gs.WindowWidth;
	int startHeight = gs.WindowHeight;

	m_Window = new Window(startWidth, startHeight, "Cooking Station");
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
	msaaSpec.Samples = gs.MsaaSamples; 
	msaaSpec.HDR = true;
	m_MsaaFBO = std::make_shared<Framebuffer>(msaaSpec);


// DO EKSPORTU ODKOMENTOWAC 

//#ifdef CS_DISTRIBUTION
//	std::filesystem::path exePath = std::filesystem::current_path();
//	VFS::Mount("assets", std::make_shared<PackageFileSystem>((exePath / "data.pak").string()));
//	VFS::Mount("shaders", std::make_shared<PackageFileSystem>((exePath / "shaders.pak").string()));
//#else
//	VFS::Mount("assets", std::make_shared<PhysicalFileSystem>("CookingStation/Assets"));
//	VFS::Mount("shaders", std::make_shared<PhysicalFileSystem>("CookingStation/Shaders"));
//#endif

// A TO ZAKOMENTOWAC...

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

////////////////////////////

	SceneManager::NewScene();
	Renderer::Init();
	Renderer2D::Init();
	AudioEngine::Init();

	GetEventBus().Subscribe<AudioSettingsChangedEvent>([](const AudioSettingsChangedEvent& e) {
		AudioEngine::SetMusicEnabled(e.MusicEnabled);
		AudioEngine::SetSoundsEnabled(e.SoundsEnabled);
		spdlog::info("Zaktualizowano audio (Muzyka: {}, Dzwieki: {})", e.MusicEnabled, e.SoundsEnabled);
		});

	GetEventBus().Subscribe<GraphicsSettingsChangedEvent>([this](const GraphicsSettingsChangedEvent&) {
		ApplyGraphicsSettings();
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

	ApplyGraphicsSettings();
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

	spdlog::shutdown();
}
void Application::ApplyGraphicsSettings()
{
	m_ApplyingGraphicsSettings = true;
	auto& settings = GraphicsSettings::Get();
	int width = settings.WindowWidth;
	int height = settings.WindowHeight;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GLFWwindow* nativeWindow = (GLFWwindow*)m_Window->GetNativeWindow();

#ifdef CS_DISTRIBUTION
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	if (settings.Fullscreen) {
		glfwSetWindowMonitor(nativeWindow, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		m_Window->SetDecorated(false);
	}
	else {
		int workX, workY, workW, workH;
		glfwGetMonitorWorkarea(monitor, &workX, &workY, &workW, &workH);

		glfwSetWindowMonitor(nativeWindow, nullptr, workX, workY, width, height, 0);
		m_Window->SetDecorated(true);

		if (width >= workW || height >= workH) {
			glfwMaximizeWindow(nativeWindow);
		}
		else {
			glfwRestoreWindow(nativeWindow);
			int xpos = workX + (workW - width) / 2;
			int ypos = workY + (workH - height) / 2;
			glfwSetWindowPos(nativeWindow, xpos, ypos);
		}
	}
#endif
	int actualWidth, actualHeight;
	glfwGetFramebufferSize(nativeWindow, &actualWidth, &actualHeight);

	width = actualWidth;
	height = actualHeight;

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

	spdlog::info("GraphicsSettings: Zastosowano MSAA x{} | FAKTYCZNY ROZMIAR: {}x{} (Fullscreen: {})",
		settings.MsaaSamples, width, height, settings.Fullscreen);

	m_ApplyingGraphicsSettings = false;
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


		if (m_GraphicsSettingsDirty)
		{
			m_GraphicsSettingsDirty = false;
			ApplyGraphicsSettings();
		}

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

void Application::SetFullscreen(bool enabled)
{
#ifdef CS_DISTRIBUTION
	if (m_ApplyingGraphicsSettings) return;
	auto& gs = GraphicsSettings::Get();
	if (gs.Fullscreen == enabled) return;

	gs.Fullscreen = enabled;

	m_GraphicsSettingsDirty = true;
#endif
}