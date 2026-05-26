#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>
#include <algorithm>

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
		ray.Direction = glm::normalize(glm::vec3(-viewMatrix[0][2], -viewMatrix[1][2], -viewMatrix[2][2]));

		return ray;
	}

	static bool Intersects(const Ray& ray, const AABB& box)
	{
		// Wyliczamy Min i Max na bie��co
		glm::vec3 min = box.center - box.extents;
		glm::vec3 max = box.center + box.extents;

		float tmin = (min.x - ray.Origin.x) / ray.Direction.x;
		float tmax = (max.x - ray.Origin.x) / ray.Direction.x;

		if (tmin > tmax) std::swap(tmin, tmax);

		float tymin = (min.y - ray.Origin.y) / ray.Direction.y;
		float tymax = (max.y - ray.Origin.y) / ray.Direction.y;

		if (tymin > tymax) std::swap(tymin, tymax);

		if ((tmin > tymax) || (tymin > tmax)) return false;

		if (tymin > tmin) tmin = tymin;
		if (tymax < tmax) tmax = tymax;

		float tzmin = (min.z - ray.Origin.z) / ray.Direction.z;
		float tzmax = (max.z - ray.Origin.z) / ray.Direction.z;

		if (tzmin > tzmax) std::swap(tzmin, tzmax);

		if ((tmin > tzmax) || (tzmin > tmax)) return false;

		return true;
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

        if (useSSA)
        {
            if (std::abs(ray.Direction.y) > 1e-6f)
            {
                float yMax = 20.0f; // Było 15.0f
                float yMin = -10.0f; // Było -5.0f

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

                    int margin = 1;
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
                                    TransformComponent* transform = transformStorage->Get(entity);
                                    if (!transform) continue;

                                    BoxColliderComponent* collider = colliderStorage ? colliderStorage->Get(entity) : nullptr;
                                    if (requireCollider && !collider) continue;

                                    glm::vec3 boundsOffset = glm::vec3(0.0f);
                                    glm::vec3 boundsSize = glm::vec3(1.0f);

                                    if (collider) {
                                        boundsOffset = collider->Offset;
                                        boundsSize = collider->Size;
                                    }

                                    glm::vec3 globalPos = { transform->WorldMatrix[3][0], transform->WorldMatrix[3][1], transform->WorldMatrix[3][2] };
                                    glm::vec3 center = globalPos + boundsOffset;
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

                                    if (Physics::Intersects(localRay, localAABB))
                                    {
                                        float dist = glm::dot(center - ray.Origin, ray.Direction);
                                        if (dist < closestDist)
                                        {
                                            closestDist = dist;
                                            closestEntity = entity;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else // Brute-Force
        {
            for (size_t it = 0; it < transformStorage->dense.size(); it++)
            {
                Entity entity = transformStorage->reverse[it];
                TransformComponent* transform = &transformStorage->dense[it];
                BoxColliderComponent* collider = colliderStorage ? colliderStorage->Get(entity) : nullptr;

                if (requireCollider && !collider) continue;

                glm::vec3 boundsOffset = glm::vec3(0.0f);
                glm::vec3 boundsSize = glm::vec3(1.0f);

                if (collider) {
                    boundsOffset = collider->Offset;
                    boundsSize = collider->Size;
                }

                glm::vec3 globalPos = { transform->WorldMatrix[3][0], transform->WorldMatrix[3][1], transform->WorldMatrix[3][2] };
                glm::vec3 center = globalPos + boundsOffset;
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

                if (Physics::Intersects(localRay, localAABB))
                {
                    float dist = glm::dot(center - ray.Origin, ray.Direction);
                    if (dist < closestDist)
                    {
                        closestDist = dist;
                        closestEntity = entity;
                    }
                }
            }
        }

        return closestEntity;
    }
};