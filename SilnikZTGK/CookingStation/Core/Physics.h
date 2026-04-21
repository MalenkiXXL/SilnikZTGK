#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>
#include <algorithm>

//"laser" z myszki
struct Ray
{
	glm::vec3 Origin; //stad wylatuje (pozycja na ekranie przeliczona na swiat)
	glm::vec3 Direction; //kierunek w ktorym patrzy kamera
};

//wirtualne pudelko kolizyjne dookola obiektu
struct AABB
{
	glm::vec3 Min; //najmniejszy punkt
	glm::vec3 Max; //najwiekszy
};

class Physics
{
public:
	//przelicza pozycje z pikseli 2d na laser w swiecie 3d
	static Ray CastRayFromMouse(float mouseX, float mouseY, float screenWidth, float screenHeight, const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix)
	{
		// 1. Zmiana wspolrzednych z pikseli na znormalizowane koordynaty
		float ndcX = (2.0f * mouseX) / screenWidth - 1.0f;
		float ndcY = 1.0f - (2.0f * mouseY) / screenHeight; // y w opengl od dolu do gory

		// 2. Punkt startowy na plaszczyznie kamery (poprawiona literówka z ndcY)
		glm::vec4 clipCoords = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);

		// 3. Odwracamy macierz widoku i projekcji (TUTAJ u¿ywamy mat4!)
		glm::mat4 invVP = glm::inverse(projectionMatrix * viewMatrix);
		glm::vec4 worldCoords = invVP * clipCoords;

		// Normalizacja
		if (worldCoords.w != 0.0f)
		{
			worldCoords.x /= worldCoords.w;
			worldCoords.y /= worldCoords.w;
			worldCoords.z /= worldCoords.w;
		}

		Ray ray;
		ray.Origin = glm::vec3(worldCoords);
		// Poniewaz mamy kamere ortograficzn¹, wszystkie lasery lec¹ idealnie równolegle
		// Kierunek lasera to po prostu kierunek, w którym patrzy kamera
		// Wyciagamy wektor 'Forward' z macierzy widoku
		ray.Direction = glm::normalize(glm::vec3(-viewMatrix[0][2], -viewMatrix[1][2], -viewMatrix[2][2]));

		return ray;
	}

	//sprawdza czy laser przecina AABB algorytmem slab method
	static bool Intersects(const Ray& ray, const AABB& box)
	{
		float tmin = (box.Min.x - ray.Origin.x) / ray.Direction.x;
		float tmax = (box.Max.x - ray.Origin.x) / ray.Direction.x;

		if (tmin > tmax)
		{
			std::swap(tmin, tmax);
		}

		float tymin = (box.Min.y - ray.Origin.y) / ray.Direction.y;
		float tymax = (box.Max.y - ray.Origin.y) / ray.Direction.y;

		if (tymin > tymax)
		{
			std::swap(tymin, tymax);
		}

		if ((tmin > tymax) || (tymin > tmax)) return false;

		if (tymin > tmin) tmin = tymin;
		if (tymax < tmax) tmax = tymax;

		float tzmin = (box.Min.z - ray.Origin.z) / ray.Direction.z;
		float tzmax = (box.Max.z - ray.Origin.z) / ray.Direction.z;

		if (tzmin > tzmax) std::swap(tzmin, tzmax);

		if ((tmin > tzmax) || (tzmin > tmax)) return false;

		return true;
	}

	static bool Intersects(const AABB& a, const AABB& b)
	{
		return (a.Min.x <= b.Max.x && a.Max.x >= b.Min.x) &&
			(a.Min.y <= b.Max.y && a.Max.y >= b.Min.y) &&
			(a.Min.z <= b.Max.z && a.Max.z >= b.Min.z);
	}

};

