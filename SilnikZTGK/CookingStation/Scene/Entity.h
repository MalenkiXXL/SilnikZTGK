#pragma once
#include <string>
#include <glm/glm.hpp>
#include <memory>
#include "CookingStation/Renderer/Model.h"

struct TransformComponent {
    glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
    glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

    glm::mat4 GetTransformMatrix() {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), Position);

        transform = glm::rotate(transform, glm::radians(Rotation.x), { 1, 0, 0 });
        transform = glm::rotate(transform, glm::radians(Rotation.y), { 0, 1, 0 });
        transform = glm::rotate(transform, glm::radians(Rotation.z), { 0, 0, 1 });

        transform = glm::scale(transform, Scale);

        return transform;
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