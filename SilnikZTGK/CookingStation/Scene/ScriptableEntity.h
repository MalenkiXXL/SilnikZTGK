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

	template<typename T>
	T* GetParentScript()
	{
		auto* rel = GetComponent<RelationshipComponent>();
		if (!rel || rel->Parent == NULL_ENTITY) return nullptr;

		auto* nsc = m_Scene->GetWorld().GetComponentByID<NativeScriptComponent>(rel->Parent);
		if (!nsc) return nullptr;

		for (auto& element : nsc->Scripts)
		{
			if (auto* script = dynamic_cast<T*>(element.Instance))
				return script;
		}
		return nullptr;
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

		auto windowSize = Input::GetWindowSize();
		float viewWidth = (float)m_Scene->GetViewportWidth();
		float viewHeight = (float)m_Scene->GetViewportHeight();

		// Jeœli okno jest zminimalizowane lub ma b³êdy
		if (viewWidth <= 0 || viewHeight <= 0) return glm::vec3(0.0f);

#ifdef CS_DISTRIBUTION
		// W dystrybucji okno to ca³y ekran. 
		// Najlepiej pobieraæ to przez nasz system wejœcia:
		auto mousePos = Input::GetMousePosition();
		float localMouseX = mousePos.first;
		float localMouseY = mousePos.second;
#else
		// W edytorze viewport jest poœrodku paneli
		auto mousePos = Input::GetMousePosition();
		float localMouseX = mousePos.first - 200.0f;
		float localMouseY = mousePos.second - 30.0f;
#endif

		float orthoSize = 10.0f * (camera->Zoom / 45.0f);
		float aspectRatio = camera->AspectRatio;

		// Wa¿ne! Projekcja musi siê zgadzaæ z RendererLayer.cpp!
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