class CreateEntityCommand : public Command {
public:
    CreateEntityCommand(World* world, const std::string& name, const std::string& path, const glm::vec3& position)
        : m_World(world), m_Name(name), m_ModelPath(path), m_Position(position) {
    }

    virtual void Execute() override {
        m_Entity = m_World->CreateEntity();

        m_World->AddComponent<TagComponent>(m_Entity, TagComponent{ m_Name });

        MeshComponent meshComp;
        meshComp.ModelPtr = AssetManager::GetModel(m_ModelPath);
        meshComp.Path = m_ModelPath;
        m_World->AddComponent<MeshComponent>(m_Entity, meshComp);

        TransformComponent transComp;

        transComp.SetPosition(m_Position);
        transComp.SetRotation(glm::vec3(0.0f));
        transComp.SetScale(glm::vec3(1.0f, 1.0f, 1.0f)); 

        m_World->AddComponent<TransformComponent>(m_Entity, transComp);

        BoxColliderComponent colliderComp;
        if (meshComp.ModelPtr && !meshComp.ModelPtr->meshes.empty()) {
            glm::vec3 minP(std::numeric_limits<float>::max());
            glm::vec3 maxP(std::numeric_limits<float>::lowest());

            for (const auto& mesh : meshComp.ModelPtr->meshes) {
                glm::vec3 meshMin = mesh.localAABB.center - mesh.localAABB.extents;
                glm::vec3 meshMax = mesh.localAABB.center + mesh.localAABB.extents;

                minP = glm::min(minP, meshMin);
                maxP = glm::max(maxP, meshMax);
            }

            colliderComp.Offset = (minP + maxP) * 0.5f;
            colliderComp.Size = (maxP - minP) * 0.5f;
        }
        m_World->AddComponent<BoxColliderComponent>(m_Entity, colliderComp);

        m_World->AddComponent<NativeScriptComponent>(m_Entity, NativeScriptComponent{});

        spdlog::info("Command: Utworzono obiekt '{}' (ID: {})", m_Name, m_Entity.id);
    }

    virtual void Undo() override {
        m_World->DestroyEntity(m_Entity);
        spdlog::info("Command: Cofniêto utworzenie obiektu (ID: {})", m_Entity.id);
    }

private:
    World* m_World;
    Entity m_Entity;

    std::string m_Name;
    std::string m_ModelPath;
    glm::vec3 m_Position;
};