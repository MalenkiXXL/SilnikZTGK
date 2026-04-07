#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Scene/SceneSerializer.h"
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Renderer/ShaderLibrary.h"
#include "CookingStation/Events/WindowEvent.h"

#include <memory>



class RendererLayer : public Layer
{
public:
	RendererLayer() : Layer("RenderLayer") {};
	~RendererLayer() = default;

	void OnAttach() override;
	void OnUpdate(Timestep ts) override;
	void OnEvent(Event& e) override;

	void SetScene(std::shared_ptr<Scene> scene) { m_ActiveScene = scene; }

protected:
	std::shared_ptr<Scene> m_ActiveScene;

private:
	bool OnWindowResize(WindowResizeEvent& e);

	ShaderLibrary m_ShaderLibrary;
	std::shared_ptr<Shader> m_Shader;

	float m_ViewportWidth = 800.0f;
	float m_ViewportHeight = 600.0f;
};


