#pragma once
#include <string>
#include <glm/glm.hpp>
#include <memory>
#include "CookingStation/Renderer/Model.h"

struct TransformComponent {
    glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
    glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };
};

struct MeshComponent {
    std::shared_ptr<Model> ModelPtr;
};

struct TagComponent {
    std::string Tag;
};

struct ClearColorComponent {
    glm::vec4 bgColor = {0.1f, 0.2f, 0.3f,0.1f};
};