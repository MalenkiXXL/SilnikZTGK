#pragma once
#include "CookingStation/Events/Event.h"
#include "CookingStation/Events/KeyEvent.h"
#include "CookingStation/Events/MouseEvent.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Events/EventBus.h"
#include "Layer.h"
#include "Window.h"
#include "CookingStation/Layers/CameraLayer/CameraLayer.h"
#include "CookingStation/Layers/AssetLayer/AssetLayer.h"
#include "CookingStation/Layers/GuiLayer/EditorGuiLayer.h"
#include "CookingStation/Layers/GuiLayer/GameGuiLayer.h"
#include "CookingStation/Layers/EditorLayer/EditorLayer.h"
#include "CookingStation/Layers/GameLayer/GameLayer.h"
#include "CookingStation/Renderer/Framebuffer.h" 
#include "CookingStation/Core/GraphicsSettings.h"
#include <memory>
#include <vector>

class Application
{
public:
	Application();
	~Application();

	inline static Application& Get() { return *s_Instance; }
	inline Window& GetWindow() { return *m_Window; }
	inline EventBus& GetEventBus() { return m_EventBus; }

	void Run(); //Glowna petla while
	void OnEvent(Event& e); //Odbiornik Eventow

	void PushLayer(Layer* layer);
	std::vector<Layer*> m_LayerStack;

	std::shared_ptr<Framebuffer> GetViewportFBO() { return m_ViewportFBO; }
	
	void SetMsaaSamples(uint32_t samples) {
		if (m_MsaaFBO) m_MsaaFBO->SetSamples(samples);
	}

	uint32_t GetMsaaSamples() const {
		return m_MsaaFBO ? m_MsaaFBO->GetSpecification().Samples : 1;
	}

	void ApplyGraphicsSettings();

private:
	bool OnWindowClose(WindowCloseEvent& e);
	bool OnWindowResize(WindowResizeEvent& e);
	bool OnKeyPressed(KeyPressedEvent& e);

	Window* m_Window; //wskaznik na okno
	bool m_Running = true;
	bool m_IgnoreNextResize = true; // Flaga do ignorowania pierwszego eventu resize po zmianie rozdzielczości

	float m_LastFrameTime = 0.0f;
	std::shared_ptr<Framebuffer> m_ViewportFBO;
	static Application* s_Instance;
	std::shared_ptr<Framebuffer> m_MsaaFBO;

	EventBus m_EventBus;
};