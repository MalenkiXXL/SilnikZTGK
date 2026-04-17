#pragma once
#include "CookingStation/Events/Event.h"
#include "CookingStation/Layers/EditorLayer/Command.h"
#include <string>
#include "CookingStation/Scene/ecs.h"
#include "CookingStation/Scene/Entity.h"
#include <glm/glm.hpp>

class EntityTransformChangedEvent : public Event {
public:
    EntityTransformChangedEvent(Entity entity, const glm::vec3& startPos, const glm::vec3& endPos)
        : m_Entity(entity), m_StartPos(startPos), m_EndPos(endPos) {
    }

    Entity GetEntity() const { return m_Entity; }
    glm::vec3 GetStartPos() const { return m_StartPos; }
    glm::vec3 GetEndPos() const { return m_EndPos; }

    static EventType GetStaticType() { return EventType::EntityTransformChanged; }

    virtual EventType GetEventType() const override { return GetStaticType(); }

private:
    Entity m_Entity;
    glm::vec3 m_StartPos;
    glm::vec3 m_EndPos;
};

class EntityDeletedEvent : public Event {
public:
    EntityDeletedEvent(Entity entity) : m_Entity(entity) {}

    Entity GetEntity() const { return m_Entity; }

    static EventType GetStaticType() { return EventType::EntityDeleted; }
    virtual EventType GetEventType() const override { return GetStaticType(); }

private:
    Entity m_Entity;
};




class DeleteEntityCommand : public Command {
public:
    DeleteEntityCommand(World* world, Entity entity)
        : m_World(world), m_Entity(entity) {

        // 1. Snapshot: Pobieramy i kopiujemy dane wszystkich komponentów, póki encja istnieje
        if (auto* tag = m_World->GetComponent<TagComponent>(m_Entity)) {
            m_HasTag = true;
            m_Tag = *tag;
        }

        if (auto* mesh = m_World->GetComponent<MeshComponent>(m_Entity)) {
            m_HasMesh = true;
            m_Mesh = *mesh;
        }

        if (auto* transform = m_World->GetComponent<TransformComponent>(m_Entity)) {
            m_HasTransform = true;
            m_Transform = *transform;
        }

        if (auto* collider = m_World->GetComponent<BoxColliderComponent>(m_Entity)) {
            m_HasCollider = true;
            m_Collider = *collider;
        }

        if (auto* script = m_World->GetComponent<NativeScriptComponent>(m_Entity)) {
            m_HasScript = true;
            // Kopiujemy tylko definicjê skryptu (InstantiateScript), 
            // Instance musi zostaæ stworzone na nowo po przywróceniu.
            m_Script.InstantiateScript = script->InstantiateScript;
        }
    }

    virtual void Execute() override {
        // Usuwamy encjê ze œwiata
        m_World->DestroyEntity(m_Entity);
        spdlog::info("Command: Usuniêto encjê ID: {}", m_Entity.id);
    }

    virtual void Undo() override {
        // 2. Przywracanie: Tworzymy now¹ encjê
        Entity newEntity = m_World->CreateEntity();

        // Aktualizujemy uchwyt, by kolejne wywo³ania Execute() wiedzia³y co usun¹æ
        m_Entity = newEntity;

        // Przywracamy zapisane komponenty
        if (m_HasTag)       m_World->AddComponent<TagComponent>(newEntity, m_Tag);
        if (m_HasMesh)      m_World->AddComponent<MeshComponent>(newEntity, m_Mesh);
        if (m_HasTransform) m_World->AddComponent<TransformComponent>(newEntity, m_Transform);
        if (m_HasCollider)  m_World->AddComponent<BoxColliderComponent>(newEntity, m_Collider);
        if (m_HasScript)    m_World->AddComponent<NativeScriptComponent>(newEntity, m_Script);

        spdlog::info("Command: Cofniêto usuniêcie encji (Nowe ID: {}, Gen: {})", m_Entity.id, m_Entity.generation);
    }

private:
    World* m_World;
    Entity m_Entity;

    // Flagi obecnoœci komponentów
    bool m_HasTag = false;
    bool m_HasMesh = false;
    bool m_HasTransform = false;
    bool m_HasCollider = false;
    bool m_HasScript = false;

    // Kontenery na dane (kopie)
    TagComponent m_Tag;
    MeshComponent m_Mesh;
    TransformComponent m_Transform;
    BoxColliderComponent m_Collider;
    NativeScriptComponent m_Script;
};

class ScenePlayEvent : public Event {
public:
    ScenePlayEvent() = default;
    virtual EventType GetEventType() const override { return EventType::ScenePlay; }
    static EventType GetStaticType() { return EventType::ScenePlay; }
};

class SceneStopEvent : public Event {
public:
    SceneStopEvent() = default;
    virtual EventType GetEventType() const override { return EventType::SceneStop; }
    static EventType GetStaticType() { return EventType::SceneStop; }
};