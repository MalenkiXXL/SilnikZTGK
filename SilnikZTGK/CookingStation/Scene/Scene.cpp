#include "CookingStation/Scene/Scene.h"
#include "ScriptableEntity.h"

Scene::Scene() {
    m_ECSWorld.RegisterComponent<TransformComponent>();
    m_ECSWorld.RegisterComponent<MeshComponent>();
    m_ECSWorld.RegisterComponent<TagComponent>();
    m_ECSWorld.RegisterComponent<ClearColorComponent>();
    m_ECSWorld.RegisterComponent<BoxColliderComponent>();
    m_ECSWorld.RegisterComponent<NativeScriptComponent>();
};
Scene::~Scene() {};

void Scene::Update(Timestep ts)
{
    auto* scriptStorage = m_ECSWorld.GetComponentVector<NativeScriptComponent>();

    if (scriptStorage)
    {
        for (size_t i = 0; i < scriptStorage->dense.size(); i++)
        {
            Entity entity = scriptStorage->reverse[i];
            auto& scriptComp = scriptStorage->dense[i];

            if (!scriptComp.Instance)
            {
                if (scriptComp.InstantiateScript)
                {
                    scriptComp.Instance = scriptComp.InstantiateScript();
                    scriptComp.Instance->m_Entity = entity;
                    scriptComp.Instance->m_Scene = this;
                    scriptComp.Instance->OnCreate();
                }
            }

            if (scriptComp.Instance)
            {
                scriptComp.Instance->OnUpdate(ts);
            }
        }
    }
}