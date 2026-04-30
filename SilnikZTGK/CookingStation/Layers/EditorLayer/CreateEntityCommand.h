class CreateEntityCommand : public Command {
public:
    // Dodajemy wskaŸnik na shader do konstruktora komendy (albo mo¿esz go pobraæ z AssetManager'a wewn¹trz)
    CreateEntityCommand(World* world, const std::string& name, const std::string& path, const glm::vec3& position, std::shared_ptr<Shader> shader)
        : m_World(world), m_Name(name), m_ModelPath(path), m_Position(position), m_Shader(shader) {
    }

    virtual void Execute() override {
        // 1. Tworzymy now¹ encjê
        m_Entity = m_World->CreateEntity();

        // 2. Dodajemy komponenty
        m_World->AddComponent<TagComponent>(m_Entity, TagComponent{ m_Name });

        // U¿ywamy jawnego wywo³ania, zak³adaj¹c ¿e doda³eœ konstruktor do MeshComponent
        MeshComponent meshComp;
        meshComp.ModelPtr = AssetManager::GetModel(m_ModelPath);
        meshComp.ShaderPtr = m_Shader; // Podpinamy shader!
        meshComp.Path = m_ModelPath;
        m_World->AddComponent<MeshComponent>(m_Entity, meshComp);

        TransformComponent transComp;
        transComp.Position = m_Position;
        transComp.Rotation = glm::vec3(0.0f);
        transComp.Scale = glm::vec3(1.0f, 1.0f, 1.0f); // Skala 1, ¿eby obiekt by³ widoczny!
        m_World->AddComponent<TransformComponent>(m_Entity, transComp);

        m_World->AddComponent<BoxColliderComponent>(m_Entity, BoxColliderComponent{});
        m_World->AddComponent<NativeScriptComponent>(m_Entity, NativeScriptComponent{});

        // 3. Podpinamy skrypt
        auto* script = m_World->GetComponent<NativeScriptComponent>(m_Entity);
        if (script) {
            script->Bind<RotationScript>("RotationScript");
        }

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
    std::shared_ptr<Shader> m_Shader; // Zapisujemy shader w komendzie
};