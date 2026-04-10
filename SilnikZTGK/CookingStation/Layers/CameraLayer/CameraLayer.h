#pragma once
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Scene/Scene.h"
#include "Camera.h"
#include "CookingStation/Events/Event.h"
#include "CookingStation/Core/Timestep.h"




class CameraLayer : public Layer
{

public:
	CameraLayer();
 	~CameraLayer();
	void OnUpdate(Timestep ts);
	virtual void OnEvent(Event& event) {};

private:
	Camera m_Camera;

};

