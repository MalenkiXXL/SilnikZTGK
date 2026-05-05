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
		// Wyliczamy Min i Max na bie¿¹co
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
};