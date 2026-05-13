#pragma once
#include "ecs.h"
#include "Scene.h"
#include "../Core/Timestep.h"
#include "../Core/Input.h"
#include "../Core/Physics.h"
#include "../Layers/CameraLayer/Camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
class ScriptableEntity
{
public:
	virtual ~ScriptableEntity() {}

	Scene* GetScene() { return m_Scene; }

	//funkcja ktora wywoluje GetComponent z ecs dla m_Entity
	template<typename T> T* GetComponent()
	{
		return m_Scene->GetWorld().GetComponent<T>(m_Entity);
	}

	template<typename T> void AddComponent(T component)
	{
		m_Scene->GetWorld().AddComponent<T>(m_Entity, component);
	}

	virtual void OnCreate() {}
	virtual void OnDestroy() {}
	virtual void OnUpdate(Timestep ts) {}
	virtual void OnCollision() {}
	virtual void OnClick() {}

protected:
	//encja do ktorej jest przypieta
	Entity m_Entity;
	//pointer na swiat
	Scene* m_Scene;

	glm::vec3 GetMouseWorldPosition()
	{
		auto* camera = m_Scene->GetCamera();
		if (!camera) return glm::vec3(0.0f);

		auto mousePos = Input::GetMousePosition();
		auto windowSize = Input::GetWindowSize();

		float localMouseX = mousePos.first - 200.0f;
		float localMouseY = mousePos.second - 30.0f;
		float viewWidth = (float)windowSize.first - 500.0f;
		float viewHeight = (float)windowSize.second - 230.0f;

		float orthoSize = 10.0f * (camera->Zoom / 45.0f);
		float aspectRatio = camera->AspectRatio;

		glm::mat4 proj3D = glm::ortho(-aspectRatio * orthoSize, aspectRatio * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
		glm::mat4 view3D = camera->GetViewMatrix();

		Ray ray = Physics::CastRayFromMouse(localMouseX, localMouseY, viewWidth, viewHeight, proj3D, view3D);

		if (std::abs(ray.Direction.y) > 1e-6f) {
			float t = -ray.Origin.y / ray.Direction.y;
			if (t > 0.0f) return ray.Origin + t * ray.Direction;
		}
		return glm::vec3(0.0f);
	}

private:
	//tylko scene moze ustawiac te prywatne zmienne
	friend class Scene;
};