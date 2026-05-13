#pragma once
#include "ecs.h"
#include "Scene.h"
#include "../Core/Timestep.h"
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

private:
	//tylko scene moze ustawiac te prywatne zmienne
	friend class Scene;
};