#pragma once
#include "ecs.h"
#include "Scene.h"
#include "../Core/Timestep.h"
class ScriptableEntity
{
public:
	virtual ~ScriptableEntity() {}

	//funkcja ktora wywoluje GetComponent z ecs dla m_Entity
	template<typename T> T* GetComponent()
	{
		return m_Scene->GetWorld().GetComponent<T>(m_Entity);
	}

	virtual void OnCreate() {}
	virtual void OnDestroy() {}
	virtual void OnUpdate(Timestep ts) {}
	virtual void OnCollision() {}

private:
	//encja do ktorej jest przypieta
	Entity m_Entity;
	//pointer na swiat
	Scene* m_Scene;

	//tylko scene moze ustawiac te prywatne zmienne
	friend class Scene;
};