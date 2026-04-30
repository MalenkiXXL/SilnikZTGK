#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Scene/SceneSerializer.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Renderer/ShaderLibrary.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Renderer/Framebuffer.h"
#include <memory>
#include <vector>
#include <string>
#include <atomic>

// Struktura odwzorowuj¹ca JSON-a
struct Quest {
	std::string Title;
	std::string Description;
	std::string DishID;
	int Portions;
	std::string Reward;
};

class RendererLayer : public Layer
{
public:
	RendererLayer() : Layer("RenderLayer") {};
	~RendererLayer() = default;

	void OnAttach() override;
	void OnUpdate(Timestep ts) override;
	void OnEvent(Event& e) override;
	void SetTargetFramebuffer(const std::shared_ptr<Framebuffer>& fbo) { m_TargetFBO = fbo; }

	void LoadQuestFromFile(const std::string& filepath);

private:
	bool OnWindowResize(WindowResizeEvent& e);

	ShaderLibrary m_ShaderLibrary;
	std::shared_ptr<Shader> m_Shader;
	std::shared_ptr<Framebuffer> m_TargetFBO;
	float m_ViewportWidth = 800.0f;
	float m_ViewportHeight = 600.0f;

	// Dane systemu questów
	std::vector<Quest> m_ActiveQuests;
	std::atomic<bool> m_IsGenerating{ false };
	std::atomic<bool> m_GenerationDone{ false };
};