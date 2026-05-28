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

    //Jeden na całą gre
    Entity gameManager = m_ECSWorld.BuildEntity().Build();

    NativeScriptComponent managerScriptComp;
    managerScriptComp.AddScript<GameManagerScript>("GameManagerScript");
    m_ECSWorld.AddComponent(gameManager, managerScriptComp);

    // Przeszukujemy wszystkie encje, które mają w sobie komponent animacji
    auto* animatorStorage = m_ECSWorld.GetComponentVector<AnimatorComponent>();
    if (animatorStorage) {
        for (auto& animComp : animatorStorage->dense) {
            // Po wejściu w tryb Play, ODBLOKOWUJEMY animacje (i TYLKO TO!)
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
                // Przesuwamy głowicę odtwarzania w czasie - to się wywołuje co klatkę!
                animComp.AnimatorInstance->UpdateAnimation(ts.GetSeconds() * animComp.PlaybackSpeed);
            }
        }
    }

    // ==========================================
    // 1. update skrytpow
    // ==========================================
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
                        instance->OnCreate(); // Uwaga: Tu ECS może realokować pamięć pod maską!
                    }
                }

                // Ponownie odpytujemy za pomocą bezpiecznego indeksu [i][s]
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

    // ==========================================
    // 2. KROK FIZYKI I KOLIZJI 
    // ==========================================

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
    // ==========================================
     // 5. DEFERRED DELETION (USUWANIE ODROCZONE)
     // ==========================================
    for (Entity e : m_EntitiesToDestroy)
    {
        // 0. Najpierw bezpiecznie niszczymy instancje skryptów!
        // Dzięki temu odpalą się wszystkie OnDestroy() w skryptach (np. wypisanie z EventBus)
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

        // 1. Odpinamy encję od rodzica, aby uchronić drzewo relacji przed zepsuciem
        RemoveParent(e);

        // 2. Usuwamy encję z siatki przestrzennej (SSA)
        for (auto& pair : m_SpartialGrid)
        {
            auto& cellEntities = pair.second;

            cellEntities.erase(
                std::remove_if(cellEntities.begin(), cellEntities.end(),
                    [&](const Entity& gridEntity) { return gridEntity.id == e.id; }),
                cellEntities.end()
            );
        }

        // 3. Następnie fizycznie niszczymy encję w ECS
        m_ECSWorld.DestroyEntity(e);
    }
    m_EntitiesToDestroy.clear();
}

void Scene::OnRuntimeStop()
{
    std::cout << "[Scene] OnRuntimeStop\n";

    auto* scriptStorage = m_ECSWorld.GetComponentVector<NativeScriptComponent>();
    if (scriptStorage) {
        // Poprawka bezpieczeństwa referencji przy usuwaniu
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
    // 1. Tworzymy nową, pustą scenę ( m_RuntimeScene)
    std::shared_ptr<Scene> newScene = std::make_shared<Scene>();

    // 2. Używamy serializera, aby zapisać obecną scenę do pliku temp
    SceneSerializer serializer(other.get());

    // Zapisujemy w folderze saves jako plik tymczasowy przez system VFS
    serializer.Serialize("assets://saves/temp_play_scene.json");

    // 3. Wczytujemy dokładnie ten sam plik do nowej sceny przez system VFS
    SceneSerializer deserializer(newScene.get());
    deserializer.Deserialize("assets://saves/temp_play_scene.json");

    // 4. Zwracamy nową, gotową do gry scenę, która jest dokładnym klonem edytora
    return newScene;
}

// 1. Rekurencyjna funkcja schodząca w głąb drzewa
void UpdateTransformTree(World& world, std::size_t entityId, const glm::mat4& parentGlobalMatrix, bool parentIsDirty) {
    auto* transform = world.GetComponentByID<TransformComponent>(entityId);
    if (!transform) return;

    bool needsUpdate = transform->IsDirty() || parentIsDirty;

    if (needsUpdate) {
        glm::mat4 localMatrix = transform->GetLocalMatrix();
        transform->WorldMatrix = parentGlobalMatrix * localMatrix;

        // Statystyka: Praca wykonana
        Renderer::GetStats().MatrixCalculations++;
    }
    else {
        // Statystyka: Praca zaoszczędzona dzięki Dirty Flag
        Renderer::GetStats().SkippedCalculations++;
    }

    // Przekazujemy macierz niżej
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

    // Iterujemy po wszystkich encjach z Transform
    for (size_t i = 0; i < transformStorage->reverse.size(); i++) {
        Entity entity = transformStorage->reverse[i];

        bool isRoot = true;

        // Sprawdzamy czy encja ma rodzica
        if (relStorage) {
            // Używamy GetByID, aby ominąć weryfikację generacji i oprzeć się na czystym indeksie
            if (auto* rel = relStorage->GetByID(entity.id)) {
                if (rel->Parent != NULL_ENTITY) {
                    isRoot = false; // Ma rodzica! Zostanie przeliczona, gdy funkcja wywoła się dla rodzica.
                }
            }
        }

        // Je?li to korze? grafu (lub samodzielny obiekt), startujemy drzewo
        if (isRoot) {
            UpdateTransformTree(world, entity.id, glm::mat4(1.0f), true);
        }
    }
}

// W Scene.h
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

    // Zabezpieczenie
    if (parent.id == NULL_ENTITY) {
        spdlog::warn("Uzyto SetParent z pustym rodzicem. Uzywaj RemoveParent!");
        RemoveParent(child);
        return;
    }

    // 1. Zabezpieczenie przed zap?tleniem (Cylic Dependency Check)
    Entity currentAncestor = parent;
    while (currentAncestor.id != NULL_ENTITY) {
        if (currentAncestor.id == child.id) {
            spdlog::warn("Nie mozna podpiac: Cykl w hierarchii! Encja {} jest juz przodkiem {}.", child.id, parent.id);
            return; // Przerywamy akcj?!
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

    // 2. Dodajemy komponenty relacji, je?li ich nie maj?
    if (!world.GetComponent<RelationshipComponent>(child)) {
        world.AddComponent<RelationshipComponent>(child, {});
    }
    if (!world.GetComponent<RelationshipComponent>(parent)) {
        world.AddComponent<RelationshipComponent>(parent, {});
    }

    auto* childRel = world.GetComponent<RelationshipComponent>(child);
    auto* parentRel = world.GetComponent<RelationshipComponent>(parent);

    // 3. NOWE PODPIĘCIE (odpięcie od starego rodzica obsługuje RemoveParent powyżej)
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

//wypleniamy strukture ssa -> dla kazdego kafelka sprawdzamy jakie ma entity w srodku
void Scene::UpdateSpatialGrid()
{
    auto* transformStorage = GetWorld().GetComponentVector<TransformComponent>();
    if (!transformStorage) return;

    for (size_t i = 0; i < transformStorage->dense.size(); ++i)
    {
        TransformComponent& transform = transformStorage->dense[i];

        // aktualizujemy przypisanie do siatki tylko jeśli obiekt zmienił pozycję
        if (transform.IsWorldDirty())
        {
            Entity entity = transformStorage->reverse[i];

            // 1. usuwamy encję z jej poprzedniego kafelka
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

            // 3. nowy klucz i dodajemy encję
            glm::ivec2 newCell = GridSystem::WorldToCell(globalPos);
            m_SpartialGrid[newCell].push_back(entity);

            // 4. czyścimy flagę
            transform.ClearWorldDirty();
        }
    }
}

//pobieramy jakie sa entity w kafelku
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

    auto* oldParentRel = world.GetComponent<RelationshipComponent>({ childRel->Parent, 0 });
    if (oldParentRel) {
        // Szukamy dziecka na liście starego rodzica i przepinamy sąsiadów
        if (oldParentRel->FirstChild == child.id) {
            oldParentRel->FirstChild = childRel->NextSibling;
        }
        if (childRel->PreviousSibling != NULL_ID) {
            auto* prevRel = world.GetComponent<RelationshipComponent>({ childRel->PreviousSibling, 0 });
            if (prevRel) prevRel->NextSibling = childRel->NextSibling;
        }
        if (childRel->NextSibling != NULL_ID) {
            auto* nextRel = world.GetComponent<RelationshipComponent>({ childRel->NextSibling, 0 });
            if (nextRel) nextRel->PreviousSibling = childRel->PreviousSibling;
        }
        oldParentRel->ChildrenCount--;
    }

    // Resetujemy relacje samego dziecka
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
    DestroyEntity(e.TargetEntity);
}

Scene::~Scene()
{
    GetWorld().GetEventBus().Unsubscribe<EntityDestroyRequestEvent>(m_DestroySubId);
}