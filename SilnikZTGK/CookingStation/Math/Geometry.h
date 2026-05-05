#pragma once
#include <glm/glm.hpp>

struct Plane {
    glm::vec3 normal = { 0.f, 1.f, 0.f };
    float distance = 0.f;
    void Normalize(); // Implementacja w pliku .cpp
};

struct Frustum {
    Plane topFace, bottomFace, rightFace, leftFace, farFace, nearFace;
};

struct AABB {
    glm::vec3 center{ 0.f };
    glm::vec3 extents{ 0.f };
};

// Funkcje pomocnicze
Frustum ExtractFrustum(const glm::mat4& viewProj);
bool IsOnFrustum(const Frustum& camFrustum, const AABB& aabb);

