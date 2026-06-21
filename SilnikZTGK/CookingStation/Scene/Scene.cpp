#include "Scene.h"
#include "ScriptableEntity.h" 
#include "Entity.h"            
#include "CookingStation/Core/Physics.h"            
#include "spdlog/spdlog.h"                  
#include "CookingStation/Scene/SceneSerializer.h"
#include <memory>
#include <algorithm>
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include "CookingStation/Renderer/Model.h"          
#include "glm/gtc/matrix_transform.hpp"
#include "CookingStation/Scripts/ConveyorBelt/ConveyorScript.h"
#include "CookingStation/Layers/AssetLayer/Animation.h"
#include "CookingStation/Layers/GameLayer/Animator.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Scripts/Delivery/DeliveryManagerScript.h"
#include <iostream> 


Scene::Scene()
{ 
    m_ECSWorld.RegisterComponent<TagComponent>();
    m_ECSWorld.RegisterComponent<MeshComponent>();
    m_ECSWorld.RegisterComponent<TransformComponent>();
    m_ECSWorld.RegisterComponent<BoxColliderComponent>();
    m_ECSWorld.RegisterComponent<NativeScriptComponent>();
    m_ECSWorld.RegisterComponent<ClearColorComponent>();
    m_ECSWorld.RegisterComponent<RelationshipComponent>();
    m_ECSWorld.RegisterComponent<UVScrollComponent>();
    m_ECSWorld.RegisterComponent<AnimatorComponent>();
    m_ECSWorld.RegisterComponent<TransformAnimatorComponent>();

    auto& bus = GetWorld().GetEventBus();

    m_DestroySubId = bus.Subscribe<EntityDestroyRequestEvent>(
        [this](const EntityDestroyRequestEvent& e) {
            this->OnEntityDestroyRequest(e);
        }
    );
}

AABB ComputeDynamicAABB(TransformComponent* trans, BoxColliderComponent* col)
{
    AABB box;
    glm::vec3 globalPos = glm::vec3(trans->WorldMatrix[3][0], trans->WorldMatrix[3][1], trans->WorldMatrix[3][2]);
    glm::vec3 center = globalPos + col->Offset;

    glm::vec3 extents = trans->GetScale() * col->Size;

    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(trans->GetRotation().x), { 1, 0, 0 })
        * glm::rotate(glm::mat4(1.0f), glm::radians(trans->GetRotation().y), { 0, 1, 0 })
        * glm::rotate(glm::mat4(1.0f), glm::radians(trans->GetRotation().z), { 0, 0, 1 });

    glm::vec3 rotatedExtents(
        std::abs(rotation[0][0]) * extents.x + std::abs(rotation[1][0]) * extents.y + std::abs(rotation[2][0]) * extents.z,
        std::abs(rotation[0][1]) * extents.x + std::abs(rotation[1][1]) * extents.y + std::abs(rotation[2][1]) * extents.z,
        std::abs(rotation[0][2]) * extents.x + std::abs(rotation[1][2]) * extents.y + std::abs(rotation[2][2]) * extents.z
    );

    box.center = center;
    box.extents = rotatedExtents;

    return box;
}


void Scene::OnRuntimeStart()
{
    std::cout << "[Scene] OnRuntimeStart\n";

    Entity gameManager = m_ECSWorld.BuildEntity().Build();

    NativeScriptComponent managerScriptComp;
    managerScriptComp.AddScript<GameManagerScript>("GameManagerScript");
    managerScriptComp.AddScript<DeliveryManagerScript>("DeliveryManagerScript");
    m_ECSWorld.AddComponent(gameManager, managerScriptComp);

    auto* animatorStorage = m_ECSWorld.GetComponentVector<AnimatorComponent>();
    if (animatorStorage) {
        for (auto& animComp : animatorStorage->dense) {
            animComp.IsPlaying = true;
        }
    }
}

void Scene::OnUpdateRuntime(Timestep ts)
{

    auto* animatorStorage = m_ECSWorld.GetComponentVector<AnimatorComponent>();
    if (animatorStorage) {
        for (auto& animComp : animatorStorage->dense) {
            if (animComp.IsPlaying && animComp.AnimatorInstance) {
                animComp.AnimatorInstance->UpdateAnimation(ts.GetSeconds() * animComp.PlaybackSpeed);
            }
        }
    }


    CalculateTransforms();
    auto* scriptStorage = m_ECSWorld.GetComponentVector<NativeScriptComponent>();

    if (scriptStorage) {
        for (size_t i = 0; i < scriptStorage->dense.size(); i++) {
            Entity entity = scriptStorage->reverse[i];

            for (size_t s = 0; s < scriptStorage->dense[i].Scripts.size(); s++) {
                if (!scriptStorage->dense[i].Scripts[s].Instance) {
                    if (scriptStorage->dense[i].Scripts[s].InstantiateScript) {
                        auto* instance = scriptStorage->dense[i].Scripts[s].InstantiateScript();
                        scriptStorage->dense[i].Scripts[s].Instance = instance;
                        instance->m_Entity = entity;
                        instance->m_Scene = this;
                        instance->OnCreate();
                    }
                }

                if (scriptStorage->dense[i].Scripts[s].Instance) {
                    scriptStorage->dense[i].Scripts[s].Instance->OnUpdate(ts);
                }
            }
        }
    }

    UpdateSpatialGrid();

    if (!m_ConveyorCacheReady)
    {
        RebuildConveyorCache();
        m_ConveyorCacheReady = true;
    }

    auto* colliderStorage = m_ECSWorld.GetComponentVector<BoxColliderComponent>();
    auto* transformStorage = m_ECSWorld.GetComponentVector<TransformComponent>();

    if (colliderStorage && transformStorage && colliderStorage->dense.size() > 1)
    {
        struct ColliderData
        {
            Entity ent;
            AABB box;
        };

        std::vector<ColliderData> activeColliders;
        activeColliders.reserve(colliderStorage->dense.size());

        for (size_t i = 0; i < colliderStorage->dense.size(); i++)
        {
            Entity ent = colliderStorage->reverse[i];
            auto* trans = transformStorage->Get(ent);
            auto* col = &colliderStorage->dense[i];

            if (trans) {
                activeColliders.push_back({ ent, ComputeDynamicAABB(trans, col) });
            }
        }


        std::sort(activeColliders.begin(), activeColliders.end(), [](const ColliderData& a, const ColliderData& b) {
            return (a.box.center.x - a.box.extents.x) < (b.box.center.x - b.box.extents.x);
            });

        for (size_t i = 0; i < activeColliders.size(); i++)
        {
            const auto& dataA = activeColliders[i];

            for (size_t j = i + 1; j < activeColliders.size(); j++)
            {
                const auto& dataB = activeColliders[j];

                if ((dataB.box.center.x - dataB.box.extents.x) > (dataA.box.center.x + dataA.box.extents.x))
                {
                    break;
                }

                if (Physics::Intersects(dataA.box, dataB.box))
                {
                    GetWorld().GetEventBus().Publish(CollisionEvent{ dataA.ent, dataB.ent });
                }
            }
        }
    }

    std::vector<Entity> queueToDestroy = m_EntitiesToDestroy;
    m_EntitiesToDestroy.clear();

    for (Entity e : queueToDestroy)
    {
        auto* scriptComp = m_ECSWorld.GetComponent<NativeScriptComponent>(e);
        if (scriptComp) {
            for (auto& scriptEl : scriptComp->Scripts) {
                if (scriptEl.Instance) {
                    scriptEl.Instance->OnDestroy();
                    if (scriptEl.DestroyScript) scriptEl.DestroyScript(&scriptEl);
                    scriptEl.Instance = nullptr;
                }
            }
        }

        RemoveParent(e);

        for (auto& pair : m_SpartialGrid)
        {
            auto& cellEntities = pair.second;

            cellEntities.erase(
                    std::remove_if(cellEntities.begin(), cellEntities.end(),
                                   [&](const Entity& gridEntity) { return gridEntity.id == e.id; }),
                    cellEntities.end()
            );
        }

        m_ECSWorld.DestroyEntity(e);
        GetWorld().GetEventBus().Publish(EntityDestroyedEvent{ e });
    }
}

void Scene::OnRuntimeStop()
{
    std::cout << "[Scene] OnRuntimeStop\n";

    auto* scriptStorage = m_ECSWorld.GetComponentVector<NativeScriptComponent>();
    if (scriptStorage) {
        for (size_t i = 0; i < scriptStorage->dense.size(); i++) {
            for (size_t s = 0; s < scriptStorage->dense[i].Scripts.size(); s++) {
                auto& scriptEl = scriptStorage->dense[i].Scripts[s];
                if (scriptEl.Instance) {
                    scriptEl.Instance->OnDestroy();
                    if (scriptEl.DestroyScript) scriptEl.DestroyScript(&scriptEl);
                    scriptEl.Instance = nullptr;
                }
            }
        }
    }

    m_ConveyorCacheReady = false;
}

std::shared_ptr<Scene> Scene::Copy(std::shared_ptr<Scene> other)
{
    std::shared_ptr<Scene> newScene = std::make_shared<Scene>();

    SceneSerializer serializer(other.get());

    serializer.Serialize("assets://saves/temp_play_scene.json");

    SceneSerializer deserializer(newScene.get());
    deserializer.Deserialize("assets://saves/temp_play_scene.json");

    return newScene;
}

void UpdateTransformTree(World& world, std::size_t entityId, const glm::mat4& parentGlobalMatrix, bool parentIsDirty) {
    auto* transform = world.GetComponentByID<TransformComponent>(entityId);
    if (!transform) return;

    bool needsUpdate = transform->IsDirty() || parentIsDirty;

    if (needsUpdate) {
        glm::mat4 localMatrix = transform->GetLocalMatrix();
        transform->WorldMatrix = parentGlobalMatrix * localMatrix;

        Renderer::GetStats().MatrixCalculations++;
    }
    else {
        Renderer::GetStats().SkippedCalculations++;
    }

    auto* rel = world.GetComponentByID<RelationshipComponent>(entityId);
    if (rel && rel->FirstChild != NULL_ENTITY) {
        std::size_t currentChildId = rel->FirstChild;

        while (currentChildId != NULL_ENTITY) {
            UpdateTransformTree(world, currentChildId, transform->WorldMatrix, needsUpdate);

            auto* childRel = world.GetComponentByID<RelationshipComponent>(currentChildId);
            if (childRel) {
                currentChildId = childRel->NextSibling;
            }
            else {
                break;
            }
        }
    }
}

void Scene::CalculateTransforms() {
    auto& world = GetWorld();
    auto* transformStorage = world.GetComponentVector<TransformComponent>();
    auto* relStorage = world.GetComponentVector<RelationshipComponent>();

    if (!transformStorage) return;

    for (size_t i = 0; i < transformStorage->reverse.size(); i++) {
        Entity entity = transformStorage->reverse[i];

        bool isRoot = true;

        if (relStorage) {
            if (auto* rel = relStorage->GetByID(entity.id)) {
                if (rel->Parent != NULL_ENTITY) {
                    isRoot = false; 
                }
            }
        }

        if (isRoot) {
            UpdateTransformTree(world, entity.id, glm::mat4(1.0f), true);
        }
    }
}

Entity Scene::GetParent(Entity child)
{
    auto* rel = m_ECSWorld.GetComponent<RelationshipComponent>(child);
    if (rel && rel->Parent != NULL_ENTITY) {
        auto* relStorage = m_ECSWorld.GetComponentVector<RelationshipComponent>();
        if (relStorage && rel->Parent < relStorage->sparse.size())
        {
            std::size_t index = relStorage->sparse[rel->Parent];
            if (index != NULL_ENTITY)
            {
                return relStorage->reverse[index];
            }
        }

        return { rel->Parent, 0 };
    }

    return { NULL_ENTITY, 0 };
}

void Scene::SetParent(Entity child, Entity parent) {
    auto& world = GetWorld();

    if (parent.id == NULL_ENTITY) {
        spdlog::warn("Uzyto SetParent z pustym rodzicem. Uzywaj RemoveParent!");
        RemoveParent(child);
        return;
    }

    Entity currentAncestor = parent;
    while (currentAncestor.id != NULL_ENTITY) {
        if (currentAncestor.id == child.id) {
            spdlog::warn("Nie mozna podpiac: Cykl w hierarchii! Encja {} jest juz przodkiem {}.", child.id, parent.id);
            return; 
        }
        auto* ancestorRel = world.GetComponent<RelationshipComponent>(currentAncestor);
        if (ancestorRel && ancestorRel->Parent != NULL_ENTITY) {
            currentAncestor.id = ancestorRel->Parent;
        }
        else {
            break;
        }
    }

    RemoveParent(child);

    if (!world.GetComponent<RelationshipComponent>(child)) {
        world.AddComponent<RelationshipComponent>(child, {});
    }
    if (!world.GetComponent<RelationshipComponent>(parent)) {
        world.AddComponent<RelationshipComponent>(parent, {});
    }

    auto* childRel = world.GetComponent<RelationshipComponent>(child);
    auto* parentRel = world.GetComponent<RelationshipComponent>(parent);

    childRel->Parent = parent.id;
    childRel->NextSibling = parentRel->FirstChild;
    childRel->PreviousSibling = NULL_ENTITY;

    if (parentRel->FirstChild != NULL_ENTITY) {
        auto* oldFirstChildRel = world.GetComponentByID<RelationshipComponent>(parentRel->FirstChild);
        if (oldFirstChildRel) {
            oldFirstChildRel->PreviousSibling = child.id;
        }
    }

    parentRel->FirstChild = child.id;
    parentRel->ChildrenCount++;

    UpdateSpatialGrid();
}

void Scene::RebuildConveyorCache()
{
    ConveyorMap.clear();

    auto* scriptStorage = m_ECSWorld.GetComponentVector<NativeScriptComponent>();
    if (!scriptStorage) return;

    for (auto& scriptComp : scriptStorage->dense)
    {
        ConveyorScript* conveyor = nullptr;
        for (auto& scriptEl : scriptComp.Scripts) {
            conveyor = dynamic_cast<ConveyorScript*>(scriptEl.Instance);
            if (conveyor) break;
        }

        if (!conveyor) continue;

        auto* t = conveyor->GetComponent<TransformComponent>();
        if (!t) continue;

        if (t->GetPosition().y < -5.0f) continue;

        GridPos key{ (int)std::round(t->GetPosition().x / 2.0f),
                     (int)std::round(t->GetPosition().z / 2.0f) };

        ConveyorMap[key] = conveyor;
    }

    spdlog::info("Zbudowano mape {} tasm.", ConveyorMap.size());
}

ConveyorScript* Scene::GetConveyorAt(float worldX, float worldZ)
{
    GridPos key{ (int)std::round(worldX / 2.0f),
                  (int)std::round(worldZ / 2.0f) };

    auto it = ConveyorMap.find(key);
    return (it != ConveyorMap.end()) ? it->second : nullptr;
}

void Scene::UpdateSpatialGrid()
{
    auto* transformStorage = GetWorld().GetComponentVector<TransformComponent>();
    if (!transformStorage) return;

    for (size_t i = 0; i < transformStorage->dense.size(); ++i)
    {
        TransformComponent& transform = transformStorage->dense[i];

        if (transform.IsWorldDirty())
        {
            Entity entity = transformStorage->reverse[i];

            for (auto& pair : m_SpartialGrid)
            {
                auto& cellEntities = pair.second;
                bool found = false;

                for (auto it = cellEntities.begin(); it != cellEntities.end(); ++it)
                {
                    if (it->id == entity.id && it->generation == entity.generation)
                    {
                        cellEntities.erase(it);
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }

            glm::vec3 globalPos = { transform.WorldMatrix[3][0], transform.WorldMatrix[3][1], transform.WorldMatrix[3][2] };

            glm::ivec2 newCell = GridSystem::WorldToCell(globalPos);
            m_SpartialGrid[newCell].push_back(entity);

            transform.ClearWorldDirty();
        }
    }
}

const std::vector<Entity>* Scene::GetEntitiesInCell(const glm::ivec2& cell) const
{
    auto it = m_SpartialGrid.find(cell);
    if (it != m_SpartialGrid.end())
    {
        return &it->second;
    }
    return nullptr;
}

void Scene::RemoveParent(Entity child) {

    auto& world = GetWorld();
    const std::size_t NULL_ID = std::numeric_limits<std::size_t>::max();

    auto* childRel = world.GetComponent<RelationshipComponent>(child);

    if (!childRel || childRel->Parent == NULL_ID) return;

    auto* oldParentRel = world.GetComponentByID<RelationshipComponent>(childRel->Parent);
    if (oldParentRel) {
        if (oldParentRel->FirstChild == child.id) {
            oldParentRel->FirstChild = childRel->NextSibling;
        }
        if (childRel->PreviousSibling != NULL_ID) {
            auto* prevRel = world.GetComponentByID<RelationshipComponent>(childRel->PreviousSibling);
            if (prevRel) prevRel->NextSibling = childRel->NextSibling;
        }
        if (childRel->NextSibling != NULL_ID) {
            auto* nextRel = world.GetComponentByID<RelationshipComponent>(childRel->NextSibling);
            if (nextRel) nextRel->PreviousSibling = childRel->PreviousSibling;
        }
        oldParentRel->ChildrenCount--;
    }

    childRel->Parent = NULL_ID;
    childRel->NextSibling = NULL_ID;
    childRel->PreviousSibling = NULL_ID;

    UpdateSpatialGrid();
}

void Scene::DestroyEntity(Entity entity)
{
    auto it = std::find_if(
        m_EntitiesToDestroy.begin(),
        m_EntitiesToDestroy.end(),
        [&](const Entity& e)
        {
            return e.id == entity.id &&
                e.generation == entity.generation;
        });

    if (it != m_EntitiesToDestroy.end())
        return;

    m_EntitiesToDestroy.push_back(entity);
}

void Scene::OnEntityDestroyRequest(const EntityDestroyRequestEvent& e) {
    spdlog::info("ENTITY DESTROY EVENT RECEIVED");
    DestroyEntityRecursive(e.TargetEntity);
}

void Scene::DestroyEntityRecursive(Entity entity)
{
    auto* rel = m_ECSWorld.GetComponent<RelationshipComponent>(entity);
    if (rel)
    {
        std::size_t currentChildId = rel->FirstChild;
        while (currentChildId != NULL_ENTITY)
        {
            auto* childRel = m_ECSWorld.GetComponentByID<RelationshipComponent>(currentChildId);
            std::size_t nextSibling = childRel ? childRel->NextSibling : NULL_ENTITY;

            DestroyEntityRecursive({ currentChildId, 0 });

            currentChildId = nextSibling;
        }
    }

    DestroyEntity(entity);
}

Scene::~Scene()
{
    GetWorld().GetEventBus().Unsubscribe<EntityDestroyRequestEvent>(m_DestroySubId);
}