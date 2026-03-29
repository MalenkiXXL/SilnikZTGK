#include "CookingStation/Scene/Scene.h"

Scene::Scene() {
    m_ECSWorld.RegisterComponent<TransformComponent>();
    m_ECSWorld.RegisterComponent<MeshComponent>();
    m_ECSWorld.RegisterComponent<TagComponent>();
    m_ECSWorld.RegisterComponent<ClearColorComponent>();
};
Scene::~Scene() {};

void Scene::Update(){}