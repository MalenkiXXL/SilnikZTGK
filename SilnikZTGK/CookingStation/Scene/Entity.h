#pragma once
#include <string>
#include <glm/glm.hpp>
#include <memory>
#include "CookingStation/Renderer/Model.h"

constexpr std::size_t NULL_ENTITY = std::numeric_limits<std::size_t>::max();

struct RelationshipComponent {
    std::size_t Parent = NULL_ENTITY;
    std::size_t FirstChild = NULL_ENTITY;
    std::size_t NextSibling = NULL_ENTITY;
    std::size_t PreviousSibling = NULL_ENTITY;

    // do szybkiego sprawdzenia iloœci dzieci
    int ChildrenCount = 0;
};

struct TransformComponent {
private:
    // Lokalna pozycja wzgledem rodzica ukryta za enkapsulacj¹
    glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 m_Rotation = { 0.0f, 0.0f, 0.0f };
    glm::vec3 m_Scale = { 1.0f, 1.0f, 1.0f };

    // Zbuforowana macierz lokalna
    glm::mat4 m_LocalMatrix = glm::mat4(1.0f);

    // FLAGI OPTYMALIZACYJNE
    bool m_IsDirty = true;        
    bool m_WorldIsDirty = true;  

public:
    // Ostateczna macierz przekazywana do renderera
    glm::mat4 WorldMatrix = glm::mat4(1.0f);

    const glm::vec3& GetPosition() const { return m_Position; }
    const glm::vec3& GetRotation() const { return m_Rotation; }
    const glm::vec3& GetScale() const { return m_Scale; }

    void SetPosition(const glm::vec3& pos) { m_Position = pos; m_IsDirty = true; m_WorldIsDirty = true; }
    void SetRotation(const glm::vec3& rot) { m_Rotation = rot; m_IsDirty = true; m_WorldIsDirty = true; }
    void SetScale(const glm::vec3& scale) { m_Scale = scale;  m_IsDirty = true; m_WorldIsDirty = true; }

    bool IsDirty() const { return m_IsDirty; }
    void SetWorldDirty() { m_WorldIsDirty = true; }
    bool IsWorldDirty() const { return m_WorldIsDirty; }
    void ClearWorldDirty() { m_WorldIsDirty = false; }

    // Funkcja licz¹ca lokaln¹ macierz ze zintegrowan¹ pamiêci¹ podrêczn¹
    const glm::mat4& GetLocalMatrix() {
        if (m_IsDirty) {
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation.x), { 1, 0, 0 })
                * glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation.y), { 0, 1, 0 })
                * glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation.z), { 0, 0, 1 });

            m_LocalMatrix = glm::translate(glm::mat4(1.0f), m_Position) * rotation * glm::scale(glm::mat4(1.0f), m_Scale);

            m_IsDirty = false; // Zdejmujemy flagê
        }
        return m_LocalMatrix;
    }
};

struct MeshComponent {
    std::shared_ptr<Model> ModelPtr = nullptr;
    std::shared_ptr<Shader> ShaderPtr = nullptr; 
    std::string Path = "";
    std::string ShaderName = "ModelShader";

    // Konstruktor domyœlny
    MeshComponent() = default;

    // Konstruktor z parametrami
    MeshComponent(std::shared_ptr<Model> model, std::shared_ptr<Shader> shader, const std::string& path = "")
        : ModelPtr(model), ShaderPtr(shader), Path(path) {
    }
};

struct TagComponent {
    std::string Tag;
};

struct ClearColorComponent {
    glm::vec4 bgColor = {0.1f, 0.2f, 0.3f,0.1f};
};

struct BoxColliderComponent
{
    glm::vec3 Size = { 1.0f, 1.0f, 1.0f };
    glm::vec3 Offset = { 0.0f, 0.0f, 0.0f };
};

struct UVScrollComponent {
    float Speed = 2.0f;  
    float Offset = 0.0f;  
};

struct ShaderComponent {
    std::shared_ptr<Shader> shader;

    ShaderComponent() = default;
    ShaderComponent(const ShaderComponent&) = default;
    ShaderComponent(std::shared_ptr<Shader> s) : shader(s) {}
};