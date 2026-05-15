#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Renderer/Texture.h"
#include <vector>
#include <glm/glm.hpp>
#include <cstdlib> // dla funkcji rand()

// Definiuje parametry startowe (jak ma zachowaæ siê cz¹steczka przy wyrzucie)
struct ParticleProps
{
    glm::vec3 PositionOffset = { 0.0f, 0.0f, 0.0f }; // Sk¹d wylatuje (wzglêdem obiektu)
    glm::vec3 Velocity = { 0.0f, 1.0f, 0.0f };       // Bazowy kierunek (np. w górê)
    glm::vec3 VelocityVariation = { 0.5f, 0.5f, 0.5f }; // Rozrzut na boki

    glm::vec4 ColorBegin = { 1.0f, 1.0f, 1.0f, 1.0f };
    glm::vec4 ColorEnd = { 1.0f, 1.0f, 1.0f, 0.0f };

    float SizeBegin = 0.5f, SizeVariation = 0.1f, SizeEnd = 0.0f;
    float LifeTime = 1.0f; // Ile sekund ¿yje

    // Kolekcja tekstur 
    std::vector<std::shared_ptr<Texture2D>> Textures;
};

// Fizyczna cz¹steczka, która ¿yje w pamiêci
struct Particle
{
    glm::vec3 Position;
    glm::vec3 Velocity;
    glm::vec4 ColorBegin, ColorEnd;

    float SizeBegin, SizeEnd;
    float LifeTime = 1.0f;
    float LifeRemaining = 0.0f;

    bool Active = false;
    uint32_t TextureID = 0;
};

class ParticleEmitterScript : public ScriptableEntity
{
public:
    ParticleProps ParticleTemplate;
    bool IsEmitting = false;
    float EmitRate = 0.05f;

private:
    std::vector<Particle> m_ParticlePool;
    uint32_t m_PoolIndex = 999;
    float m_TimeSinceLastEmit = 0.0f;

public:
    void OnCreate() override
    {
        // TYLKO alokacja pamiêci dla puli
        m_ParticlePool.resize(1000);
        m_PoolIndex = 999;
        IsEmitting = false;
    }

    void Play() { IsEmitting = true; }
    void Stop() { IsEmitting = false; }

    void Clear()
    {
        for (auto& particle : m_ParticlePool)
        {
            particle.Active = false;
        }
    }

    void OnUpdate(Timestep ts) override
    {
        if (IsEmitting) {
            m_TimeSinceLastEmit += ts.GetSeconds();
            while (m_TimeSinceLastEmit > EmitRate) {
                Emit();
                m_TimeSinceLastEmit -= EmitRate;
            }
        }

        for (auto& particle : m_ParticlePool)
        {
            if (!particle.Active) continue;

            if (particle.LifeRemaining <= 0.0f)
            {
                particle.Active = false;
                continue;
            }

            particle.LifeRemaining -= ts.GetSeconds();
            particle.Position += particle.Velocity * ts.GetSeconds();
        }
    }

    const std::vector<Particle>& GetParticles() const { return m_ParticlePool; }

private:
    float RandomFloat() {
        return ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    }

    void Emit()
    {
        Particle& particle = m_ParticlePool[m_PoolIndex];
        particle.Active = true;
        particle.LifeRemaining = ParticleTemplate.LifeTime;
        particle.LifeTime = ParticleTemplate.LifeTime;

        if (!ParticleTemplate.Textures.empty())
        {
            int texIndex = rand() % ParticleTemplate.Textures.size();
            if (ParticleTemplate.Textures[texIndex])
            {
                particle.TextureID = ParticleTemplate.Textures[texIndex]->GetRendererID();
            }
            else particle.TextureID = 0;
        }
        else particle.TextureID = 0;

        auto* transform = GetComponent<TransformComponent>();
        glm::vec3 globalPos = glm::vec3(0.0f);
        if (transform) {
            // Wyci¹gamy wektor translacji prosto z wymno¿onej macierzy œwiata
            globalPos = glm::vec3(transform->WorldMatrix[3][0], transform->WorldMatrix[3][1], transform->WorldMatrix[3][2]);
        }

        particle.Position = globalPos + ParticleTemplate.PositionOffset;

        particle.Velocity = ParticleTemplate.Velocity;
        particle.Velocity.x += ParticleTemplate.VelocityVariation.x * RandomFloat();
        particle.Velocity.y += ParticleTemplate.VelocityVariation.y * RandomFloat();
        particle.Velocity.z += ParticleTemplate.VelocityVariation.z * RandomFloat();

        particle.ColorBegin = ParticleTemplate.ColorBegin;
        particle.ColorEnd = ParticleTemplate.ColorEnd;
        particle.SizeBegin = ParticleTemplate.SizeBegin + ParticleTemplate.SizeVariation * RandomFloat();
        particle.SizeEnd = ParticleTemplate.SizeEnd;

        if (m_PoolIndex == 0) m_PoolIndex = m_ParticlePool.size() - 1;
        else m_PoolIndex--;
    }
};