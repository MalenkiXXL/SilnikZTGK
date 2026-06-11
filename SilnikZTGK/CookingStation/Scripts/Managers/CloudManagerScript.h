#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scene/PrefabSerializer.h"
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include <vector>
#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct CloudData
{
    Entity Id;      // tylko ID, bez TC*
    float  Speed;
};

class CloudManagerScript : public ScriptableEntity
{
private:
    std::vector<CloudData> m_Clouds;

    int   m_CloudCount = 10;
    float m_Height     = -15.0f;

    Camera* m_CachedCamera = nullptr;

    std::mt19937                          m_Rng{ std::random_device{}() };
    std::uniform_real_distribution<float> m_Dist01{ 0.0f, 1.0f };

    static constexpr glm::vec3 MOVE_DIR = { 0.70711f, 0.0f, -0.70711f };

public:
    void OnCreate() override
    {
        m_CachedScene = SceneManager::GetActiveScene();
        if (!m_CachedScene) return;
        auto& world = m_CachedScene->GetWorld();

        m_Clouds.reserve(m_CloudCount);

        const float initRange = 60.0f;

        for (int i = 0; i < m_CloudCount; i++)
        {
            float x = -initRange + Rand() * (initRange * 2.0f);
            float z = -initRange + Rand() * (initRange * 2.0f);
            glm::vec3 spawnPos{ x, m_Height, z };

            Entity cloudEntity = PrefabSerializer::Deserialize(
                m_CachedScene.get(), "CookingStation/Assets/prefabs/cloud.json", spawnPos)[0];

            if (!world.GetComponent<TagComponent>(cloudEntity))
                world.AddComponent<TagComponent>(cloudEntity, TagComponent{ "Cloud" });

            auto* mc = world.GetComponent<MeshComponent>(cloudEntity);
            if (mc) mc->ShaderName = "CloudShader";

            auto* tc = world.GetComponent<TransformComponent>(cloudEntity);
            float scale = 0.5f + Rand() * 1.0f;
            float speed = (2.0f - scale) + Rand() * 0.2f;
            if (tc) tc->SetScale(glm::vec3(scale));

            m_Clouds.push_back({ cloudEntity, speed });
        }
    }

    void OnUpdate(Timestep ts) override {
        if (!m_CachedScene)
            m_CachedScene = SceneManager::GetActiveScene();
        if (!m_CachedScene) return;

        m_CachedCamera = m_CachedScene->GetCamera();
        if (!m_CachedCamera) return;

        auto &world = m_CachedScene->GetWorld();


        // 1. ZNALEZIENIE ŚRODKA EKRANU NA PŁASZCZYŹNIE CHMUR
        glm::vec3 camPos = m_CachedCamera->Position;
        glm::vec3 camFront = m_CachedCamera->Front;

        glm::vec3 planeCenter = glm::vec3(0.0f, m_Height, 0.0f);
        if (std::abs(camFront.y) > 0.001f) {
            float t = (m_Height - camPos.y) / camFront.y;
            planeCenter = camPos + camFront * t;
        }

        // 2. WYLICZENIE GRANIC (BOUNDING BOX) NA BAZIE ZOOMU KAMERY (OrthoSize)
        float viewportW = (float) m_CachedScene->GetViewportWidth();
        float viewportH = (float) m_CachedScene->GetViewportHeight();
        float aspect = (viewportH > 0.0f) ? (viewportW / viewportH) : 1.777f;

        float orthoSize = m_CachedCamera->OrthoSize;

        // Marginesy ukrywające doczytywanie
        float radiusX = orthoSize * aspect * 2.5f;
        float radiusZ = orthoSize * 3.0f;

        float minX = planeCenter.x - radiusX;
        float maxX = planeCenter.x + radiusX;
        float minZ = planeCenter.z - radiusZ;
        float maxZ = planeCenter.z + radiusZ;

        const float dt = static_cast<float>(ts);

        for (auto &cloud: m_Clouds) {
            auto *tc = world.GetComponent<TransformComponent>(cloud.Id);
            if (!tc) continue;

            glm::vec3 pos = tc->GetPosition();
            pos += MOVE_DIR * cloud.Speed * dt;

            if (pos.x > maxX) {
                pos.x = minX;
                pos.z = minZ + Rand() * (maxZ - minZ);
            } else if (pos.x < minX) {
                pos.x = maxX;
                pos.z = minZ + Rand() * (maxZ - minZ);
            }

            if (pos.z > maxZ) pos.z = minZ;
            else if (pos.z < minZ) pos.z = maxZ;

            tc->SetPosition(pos);
        }
    }

private:
    inline float Rand() { return m_Dist01(m_Rng); }
    std::shared_ptr<Scene> m_CachedScene;
};