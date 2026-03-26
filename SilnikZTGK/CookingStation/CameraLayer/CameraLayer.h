#pragma once
#include <../SilnikZTGK/CookingStation/Layer.h>
#include <../SilnikZTGK/CookingStation/Camera.h>
class CameraLayer : public Layer
{

public:
	CameraLayer();
 	~CameraLayer();
	void OnUpdate();
	virtual void OnEvent(Event& event) {};

private:
	Camera m_Camera;
};

