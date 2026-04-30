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
    // lokalna pozycja wzgledem rodzica
    glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
    glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

    // ostateczna macierz przekazywana do renderera
    glm::mat4 WorldMatrix = glm::mat4(1.0f);

    // funkcja licz¹ca lokaln¹ macierz (Translation * Rotation * Scale)
    glm::mat4 GetLocalMatrix() const {
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.x), { 1, 0, 0 })
            * glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.y), { 0, 1, 0 })
            * glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.z), { 0, 0, 1 });

        return glm::translate(glm::mat4(1.0f), Position) * rotation * glm::scale(glm::mat4(1.0f), Scale);
    }
};

struct MeshComponent {
    std::shared_ptr<Model> ModelPtr = nullptr;
    std::shared_ptr<Shader> ShaderPtr = nullptr; 
    std::string Path = "";

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

struct ConveyorComponent
{
    glm::vec3 Direction = { 1.0f, 0.0f, 0.0f };
    float Speed = 2.0f;
};