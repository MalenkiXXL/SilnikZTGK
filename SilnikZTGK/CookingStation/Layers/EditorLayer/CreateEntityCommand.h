class CreateEntityCommand : public Command {
public:
    // Dodajemy wskaŸnik na shader do konstruktora komendy (albo mo¿esz go pobraæ z AssetManager'a wewn¹trz)
    CreateEntityCommand(World* world, const std::string& name, const std::string& path, const glm::vec3& position)
        : m_World(world), m_Name(name), m_ModelPath(path), m_Position(position) {
    }

    virtual void Execute() override {
        // 1. Tworzymy now¹ encjê
        m_Entity = m_World->CreateEntity();

        // 2. Dodajemy komponenty
        m_World->AddComponent<TagComponent>(m_Entity, TagComponent{ m_Name });

        // U¿ywamy jawnego wywo³ania, zak³adaj¹c ¿e doda³eœ konstruktor do MeshComponent
        MeshComponent meshComp;
        meshComp.ModelPtr = AssetManager::GetModel(m_ModelPath);
        meshComp.Path = m_ModelPath;
        m_World->AddComponent<MeshComponent>(m_Entity, meshComp);

        TransformComponent transComp;

        transComp.SetPosition(m_Position);
        transComp.SetRotation(glm::vec3(0.0f));
        transComp.SetScale(glm::vec3(1.0f, 1.0f, 1.0f)); // Skala 1, ¿eby obiekt by³ widoczny!

        m_World->AddComponent<TransformComponent>(m_Entity, transComp);

        BoxColliderComponent colliderComp;
        if (meshComp.ModelPtr && !meshComp.ModelPtr->meshes.empty()) {
            glm::vec3 minP(std::numeric_limits<float>::max());
            glm::vec3 maxP(std::numeric_limits<float>::lowest());

            // Szukamy skrajnych punktów ze wszystkich sub-siatek modelu
            for (const auto& mesh : meshComp.ModelPtr->meshes) {
                glm::vec3 meshMin = mesh.localAABB.center - mesh.localAABB.extents;
                glm::vec3 meshMax = mesh.localAABB.center + mesh.localAABB.extents;

                minP = glm::min(minP, meshMin);
                maxP = glm::max(maxP, meshMax);
            }

            // Ustawiamy œrodek pude³ka wzglêdem pivota modelu
            colliderComp.Offset = (minP + maxP) * 0.5f;
            // Ustawiamy Extents (po³owê rozmiaru), z których korzysta nasza fizyka
            colliderComp.Size = (maxP - minP) * 0.5f;
        }
        m_World->AddComponent<BoxColliderComponent>(m_Entity, colliderComp);
        // -------------------------------------------------------------

        m_World->AddComponent<NativeScriptComponent>(m_Entity, NativeScriptComponent{});

        // 3. Podpinamy skrypt
    /* auto* script = m_World->GetComponent<NativeScriptComponent>(m_Entity);
        if (script) {
            script->Bind<RotationScript>("RotationScript");
        }*/

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