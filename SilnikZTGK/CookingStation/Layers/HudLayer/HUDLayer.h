#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Layers/GuiLayer/Renderer2D.h"
#include "CookingStation/Core/Texture.h"
#include "CookingStation/Events/WindowEvent.h"
#include <memory>

class HUDLayer : public Layer {
public:
	HUDLayer() : Layer("HUDLayer") {}

	virtual void OnAttach() override;
	virtual void OnUpdate(Timestep ts) override;
	virtual void OnEvent(Event& e) override;

private:
	bool OnWindowResize(WindowResizeEvent& e);
	float m_ViewportWidth = 0.0f;
	float m_ViewportHeight = 0.0f;
	glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);

	std::shared_ptr<Texture> m_HeartIcon;
	std::shared_ptr<Texture> m_BackgroundBar;
};