#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include <vector>
#include <glm/glm.hpp>
#include <cstdlib> // dla funkcji rand()

// Definiuje parametry startowe (jak ma zachowaæ siê cz¹steczka przy wyrzucie)
struct ParticleProps
{
    glm::vec3 PositionOffset = { 0.0f, 0.0f, 0.0f }; // Sk¹d wylatuje (wzglêdem obiektu)
    glm::vec3 Velocity = { 0.0f, 2.0f, 0.0f };       // Bazowy kierunek (np. w górê)
    glm::vec3 VelocityVariation = { 1.0f, 0.5f, 1.0f }; // Rozrzut na boki

    glm::vec4 ColorBegin = { 0.8f, 0.8f, 0.8f, 1.0f };  // Kolor startowy (np. bia³a para)
    glm::vec4 ColorEnd = { 1.0f, 1.0f, 1.0f, 0.0f };    // Kolor koñcowy (np. zanika do przezroczystoœci)

    float SizeBegin = 0.5f, SizeVariation = 0.1f, SizeEnd = 0.0f;
    float LifeTime = 2.0f; // Ile sekund ¿yje
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

    bool Active = false; // Czy jest aktualnie wyœwietlana?
};


class ParticleEmitterScript : public ScriptableEntity
{
public:
    ParticleProps ParticleTemplate;
    bool IsEmitting = false;
    float EmitRate = 0.05f; // Czas w sekundach miêdzy kolejnymi cz¹steczkami

private:
    std::vector<Particle> m_ParticlePool;
    uint32_t m_PoolIndex = 999;
    float m_TimeSinceLastEmit = 0.0f;

public:
    void OnCreate() override
    {
        // Inicjalizujemy pulê obiektów na sztywno
        m_ParticlePool.resize(1000);
        m_PoolIndex = 999;

        // Z góry zak³adamy, ¿e dany obiekt od razu dymi
        IsEmitting = true;
    }

    void OnUpdate(Timestep ts) override
    {
        // 1. SPRAWDZAMY CZY TRZEBA WYPUŒCIÆ NOW¥ CZ¥STECZKÊ
        if (IsEmitting) {
            m_TimeSinceLastEmit += ts.GetSeconds();
            while (m_TimeSinceLastEmit > EmitRate) {
                Emit();
                m_TimeSinceLastEmit -= EmitRate;
            }
        }

        // 2. AKTUALIZUJEMY FIZYKÊ ¯YJ¥CYCH CZ¥STECZEK
        for (auto& particle : m_ParticlePool)
        {
            if (!particle.Active) continue;

            if (particle.LifeRemaining <= 0.0f)
            {
                particle.Active = false;
                continue;
            }

            particle.LifeRemaining -= ts.GetSeconds();

            // Prosta fizyka - ruch jednostajny z ewentualnym opadem grawitacyjnym
            particle.Position += particle.Velocity * ts.GetSeconds();

            // Opcjonalnie: Symulacja zanikania prêdkoœci (opór powietrza)
            // particle.Velocity.x *= 0.98f;
            // particle.Velocity.z *= 0.98f;
        }
    }

    const std::vector<Particle>& GetParticles() const { return m_ParticlePool; }

private:
    // Szybka funkcja do losowania floatów od -1.0 do 1.0
    float RandomFloat() {
        return ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    }

    void Emit()
    {
        Particle& particle = m_ParticlePool[m_PoolIndex];
        particle.Active = true;
        particle.LifeRemaining = ParticleTemplate.LifeTime;
        particle.LifeTime = ParticleTemplate.LifeTime;

        // Jeœli obiekt siê przesuwa (np. maszyna leci na taœmie), dym leci z nim
        auto* transform = GetComponent<TransformComponent>();
        glm::vec3 globalPos = transform ? transform->GetPosition() : glm::vec3(0.0f);

        particle.Position = globalPos + ParticleTemplate.PositionOffset;

        // Dodajemy losowoœæ do kierunku wylotu
        particle.Velocity = ParticleTemplate.Velocity;
        particle.Velocity.x += ParticleTemplate.VelocityVariation.x * RandomFloat();
        particle.Velocity.y += ParticleTemplate.VelocityVariation.y * RandomFloat();
        particle.Velocity.z += ParticleTemplate.VelocityVariation.z * RandomFloat();

        // Kolory i rozmiary
        particle.ColorBegin = ParticleTemplate.ColorBegin;
        particle.ColorEnd = ParticleTemplate.ColorEnd;
        particle.SizeBegin = ParticleTemplate.SizeBegin + ParticleTemplate.SizeVariation * RandomFloat();
        particle.SizeEnd = ParticleTemplate.SizeEnd;

        // Indeks schodzi w dó³ (999, 998, 997...).
        // Kiedy dojdzie do zera i spróbuje wejœæ na -1, modulo (%) zawinie go z powrotem na górê listy!
        // Nadpisujemy zawsze najstarsz¹ cz¹steczkê now¹.
        m_PoolIndex = --m_PoolIndex % m_ParticlePool.size();
    }
};