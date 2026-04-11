class MoveEntityCommand : public Command {
public:
    MoveEntityCommand(World* world, Entity entity, const glm::vec3& oldPosition, const glm::vec3& newPosition)
        : m_World(world), m_EntityId(entity.id), m_OldPosition(oldPosition), m_NewPosition(newPosition) {
    }

    virtual void Execute() override {
        // U¿ywamy nowej metody pobierania po ID
        TransformComponent* transform = m_World->GetComponentByID<TransformComponent>(m_EntityId);
        if (transform != nullptr) {
            transform->Position = m_NewPosition;
        }
    }

    virtual void Undo() override {
        TransformComponent* transform = m_World->GetComponentByID<TransformComponent>(m_EntityId);
        if (transform != nullptr) {
            transform->Position = m_OldPosition;
        }
    }

private:
    World* m_World;
    std::size_t m_EntityId; 
    glm::vec3 m_OldPosition;
    glm::vec3 m_NewPosition;
};