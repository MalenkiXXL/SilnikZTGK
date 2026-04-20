#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Layers/GuiLayer/Renderer2D.h"
#include "CookingStation/Core/Texture.h"
#include <memory>

class HUDLayer : public Layer {
public:
	HUDLayer() : Layer("HUDLayer") {}

	virtual void OnAttach() override;
	virtual void OnUpdate(Timestep ts) override;

private:
	std::shared_ptr<Texture> m_HeartIcon;
	std::shared_ptr<Texture> m_BackgroundBar;
};