#include "CookingStation/Scene/Scene.h"

Scene::Scene() {
    m_ECSWorld.RegisterComponent<TransformComponent>();
    m_ECSWorld.RegisterComponent<MeshComponent>();
    m_ECSWorld.RegisterComponent<TagComponent>();
    m_ECSWorld.RegisterComponent<ClearColorComponent>();
    m_ECSWorld.RegisterComponent<BoxColliderComponent>();   
};
Scene::~Scene() {};

void Scene::Update(){}