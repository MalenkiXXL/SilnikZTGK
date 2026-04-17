#pragma once

#include "CookingStation/Events/Event.h"
#include "CookingStation/Core/Timestep.h"
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Scene/Scene.h"
#include <memory>

class GameLayer : public Layer
{
public:
    GameLayer() : Layer("GameLayer") {}
    virtual ~GameLayer() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(Timestep ts) override;
    virtual void OnEvent(Event& e) override;

private:
    std::shared_ptr<Scene> m_ActiveScene;
};