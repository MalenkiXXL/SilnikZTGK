#pragma once
#include "Command.h"
#include "CookingStation/Scene/ecs.h"
#include "CookingStation/Scene/Entity.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scripts/RotationScript.h"
#include <string>
#include <glm/glm.hpp>


class CreateEntityCommand : public Command {
public:
    CreateEntityCommand(World* world, const std::string& name, const std::string& path, const glm::vec3& position)
        : m_World(world), m_Name(name), m_ModelPath(path), m_Position(position) {
    }

    virtual void Execute() override {
        // 1. Tworzymy now¹ encjê
        m_Entity = m_World->CreateEntity();

        // 2. Dodajemy komponenty (dok³adnie to, co mia³eœ w EditorLayer)
        m_World->AddComponent<TagComponent>(m_Entity, TagComponent{ m_Name });
        m_World->AddComponent<MeshComponent>(m_Entity, MeshComponent{ AssetManager::GetModel(m_ModelPath), m_ModelPath });

        // Ustawiamy pozycjê, któr¹ otrzymaliœmy przy klikniêciu myszk¹
        m_World->AddComponent<TransformComponent>(m_Entity, TransformComponent{ m_Position });

        m_World->AddComponent<BoxColliderComponent>(m_Entity, BoxColliderComponent{});
        m_World->AddComponent<NativeScriptComponent>(m_Entity, NativeScriptComponent{});

        // 3. Podpinamy skrypt
        auto* script = m_World->GetComponent<NativeScriptComponent>(m_Entity);
        if (script) {
            script->Bind<RotationScript>();
        }

        spdlog::info("Command: Utworzono obiekt '{}' (ID: {})", m_Name, m_Entity.id);
    }

    virtual void Undo() override {
        // Cofanie akcji: po prostu niszczymy tê encjê w œwiecie
        m_World->DestroyEntity(m_Entity);
        spdlog::info("Command: Cofniêto utworzenie obiektu (ID: {})", m_Entity.id);
    }

private:
    World* m_World;
    Entity m_Entity; // Zapamiêtujemy ID stworzonego obiektu, by móc go potem usun¹æ!

    std::string m_Name;
    std::string m_ModelPath;
    glm::vec3 m_Position;
};