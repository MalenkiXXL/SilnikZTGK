#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>
#include <algorithm>
#include <limits>

#include "CookingStation/Math/Geometry.h"

//"laser" z myszki
struct Ray
{
    glm::vec3 Origin;
    glm::vec3 Direction;
};

class Physics
{
public:
    static Ray CastRayFromMouse(float mouseX, float mouseY, float screenWidth, float screenHeight, const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix)
    {
        float ndcX = (2.0f * mouseX) / screenWidth - 1.0f;
        float ndcY = 1.0f - (2.0f * mouseY) / screenHeight;

        glm::vec4 clipCoords = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);

        glm::mat4 invVP = glm::inverse(projectionMatrix * viewMatrix);
        glm::vec4 worldCoords = invVP * clipCoords;

        if (worldCoords.w != 0.0f)
        {
            worldCoords.x /= worldCoords.w;
            worldCoords.y /= worldCoords.w;
            worldCoords.z /= worldCoords.w;
        }

        Ray ray;
        ray.Origin = glm::vec3(worldCoords);
        // Twój idealny wektor dla kamery ortograficznej - NIE RUSZAĆ
        ray.Direction = glm::normalize(glm::vec3(-viewMatrix[0][2], -viewMatrix[1][2], -viewMatrix[2][2]));

        return ray;
    }

    // Nowa, kuloodporna metoda AABB odporna na wartości NaN i dzielenie przez ZERO
    static bool Intersects(const Ray& ray, const AABB& box, float& outDist)
    {
        float tmin = -std::numeric_limits<float>::infinity();
        float tmax = std::numeric_limits<float>::infinity();

        glm::vec3 min = box.center - box.extents;
        glm::vec3 max = box.center + box.extents;

        for (int i = 0; i < 3; ++i)
        {
            if (std::abs(ray.Direction[i]) < 1e-8f)
            {
                // Promień równoległy do płaszczyzny
                if (ray.Origin[i] < min[i] || ray.Origin[i] > max[i])
                    return false;
            }
            else
            {
                float invD = 1.0f / ray.Direction[i];
                float t0 = (min[i] - ray.Origin[i]) * invD;
                float t1 = (max[i] - ray.Origin[i]) * invD;

                if (invD < 0.0f) std::swap(t0, t1);

                tmin = std::max(tmin, t0);
                tmax = std::min(tmax, t1);

                if (tmax < tmin) return false;
            }
        }

        if (tmax < 0.0f) return false;

        outDist = tmin < 0.0f ? tmax : tmin;
        return true;
    }

    static bool Intersects(const Ray& ray, const AABB& box)
    {
        float dummy;
        return Intersects(ray, box, dummy);
    }

    static bool Intersects(const AABB& a, const AABB& b)
    {
        glm::vec3 aMin = a.center - a.extents;
        glm::vec3 aMax = a.center + a.extents;
        glm::vec3 bMin = b.center - b.extents;
        glm::vec3 bMax = b.center + b.extents;

        return (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
            (aMin.y <= bMax.y && aMax.y >= bMin.y) &&
            (aMin.z <= bMax.z && aMax.z >= bMin.z);
    }

    static Entity GetHoveredEntity(const Ray& ray, std::shared_ptr<Scene> activeScene, bool requireCollider, bool useSSA)
    {
        auto& world = activeScene->GetWorld();
        auto* colliderStorage = world.GetComponentVector<BoxColliderComponent>();
        auto* transformStorage = world.GetComponentVector<TransformComponent>();

        Entity closestEntity = { std::numeric_limits<std::size_t>::max(), 0 };
        float closestDist = std::numeric_limits<float>::max();

        if (!transformStorage) return closestEntity;

        // Lambda testująca konkretną encję
        auto checkEntity = [&](Entity entity) {
            TransformComponent* transform = transformStorage->Get(entity);
            if (!transform) return;

            BoxColliderComponent* collider = colliderStorage ? colliderStorage->Get(entity) : nullptr;
            if (requireCollider && !collider) return;

            glm::vec3 boundsOffset = glm::vec3(0.0f);
            glm::vec3 boundsSize = glm::vec3(1.0f);

            if (collider) {
                boundsOffset = collider->Offset;
                boundsSize = collider->Size;
            }

            glm::vec3 globalPos = { transform->WorldMatrix[3][0], transform->WorldMatrix[3][1], transform->WorldMatrix[3][2] };
            glm::vec3 center = globalPos + boundsOffset;

            // Zachowujemy Twoje oryginalne skalowanie - ufać edytorowi!
            glm::vec3 extents = transform->GetScale() * boundsSize;

            glm::vec3 rot = transform->GetRotation();
            glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), glm::radians(rot.x), { 1, 0, 0 })
                * glm::rotate(glm::mat4(1.0f), glm::radians(rot.y), { 0, 1, 0 })
                * glm::rotate(glm::mat4(1.0f), glm::radians(rot.z), { 0, 0, 1 });

            glm::mat4 obbTransform = glm::translate(glm::mat4(1.0f), center) * rotMat;
            glm::mat4 invTransform = glm::inverse(obbTransform);

            Ray localRay;
            localRay.Origin = glm::vec3(invTransform * glm::vec4(ray.Origin, 1.0f));
            localRay.Direction = glm::normalize(glm::vec3(invTransform * glm::vec4(ray.Direction, 0.0f)));

            AABB localAABB;
            localAABB.center = glm::vec3(0.0f);
            localAABB.extents = extents;

            float tHit;
            if (Physics::Intersects(localRay, localAABB, tHit))
            {
                // Obliczamy punkt uderzenia w lokalnej przestrzeni modelu
                glm::vec3 localHitPoint = localRay.Origin + localRay.Direction * tHit;

                // Konwertujemy dokładnie ten punkt zderzenia na współrzędne świata
                glm::vec3 worldHitPoint = glm::vec3(obbTransform * glm::vec4(localHitPoint, 1.0f));

                // Mierzymy odległość nie do środka uciekającego modelu, a prosto w fizyczną ściankę
                float dist = glm::distance(ray.Origin, worldHitPoint);

                if (dist < closestDist)
                {
                    closestDist = dist;
                    closestEntity = entity;
                }
            }
            };

        if (useSSA)
        {
            if (std::abs(ray.Direction.y) > 1e-6f)
            {
                float yMax = 15.0f;
                float yMin = -5.0f;

                float tTop = (yMax - ray.Origin.y) / ray.Direction.y;
                float tBottom = (yMin - ray.Origin.y) / ray.Direction.y;

                if (tTop > tBottom) std::swap(tTop, tBottom);

                if (tBottom > 0.0f)
                {
                    if (tTop < 0.0f) tTop = 0.0f;

                    glm::vec3 hitTop = ray.Origin + tTop * ray.Direction;
                    glm::vec3 hitBottom = ray.Origin + tBottom * ray.Direction;

                    glm::ivec2 cellTop = GridSystem::WorldToCell(hitTop);
                    glm::ivec2 cellBottom = GridSystem::WorldToCell(hitBottom);

                    // Potężny margines dla długich pasów transmisyjnych i zwrotnic
                    int margin = 4;
                    int minX = std::min(cellTop.x, cellBottom.x) - margin;
                    int maxX = std::max(cellTop.x, cellBottom.x) + margin;
                    int minZ = std::min(cellTop.y, cellBottom.y) - margin;
                    int maxZ = std::max(cellTop.y, cellBottom.y) + margin;

                    for (int cx = minX; cx <= maxX; cx++)
                    {
                        for (int cz = minZ; cz <= maxZ; cz++)
                        {
                            glm::ivec2 currentCell = { cx, cz };
                            const auto* entitiesInCell = activeScene->GetEntitiesInCell(currentCell);

                            if (entitiesInCell)
                            {
                                for (Entity entity : *entitiesInCell)
                                {
                                    checkEntity(entity);
                                }
                            }
                        }
                    }
                }
            }
        }

        // ZABEZPIECZENIE OSTATECZNE: Jeżeli SSA zawiodło (obiekt wyjechał z marginesu, był gigantyczny itp.)
        // odpalamy cicho pętlę awaryjną na wszystkie obiekty, aby upewnić się, że nie stracisz kliknięcia
        if (closestEntity.id == std::numeric_limits<std::size_t>::max())
        {
            for (size_t it = 0; it < transformStorage->dense.size(); it++)
            {
                Entity entity = transformStorage->reverse[it];
                checkEntity(entity);
            }
        }

        return closestEntity;
    }
};