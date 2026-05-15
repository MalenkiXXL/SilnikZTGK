#pragma once
#include "CookingStation/Scripts/ParticleEmitterScript.h"

class DustEmitterScript : public ParticleEmitterScript
{
public:
    void OnCreate() override
    {
        ParticleEmitterScript::OnCreate();

        ParticleTemplate.Textures.push_back(std::make_shared<Texture2D>("CookingStation/Assets/particles/PotParticle.png"));

        // parametry:
        ParticleTemplate.PositionOffset = { 0.0f, -0.5f, 0.0f }; 

        ParticleTemplate.Velocity = { 0.0f, 0.2f, 0.0f };       
        ParticleTemplate.VelocityVariation = { 2.5f, 0.1f, 2.5f }; 

        ParticleTemplate.ColorBegin = { 1.0f, 1.0f, 1.0f, 1.0f };
        ParticleTemplate.ColorEnd = { 1.0f, 1.0f, 1.0f, 0.9f };

        ParticleTemplate.SizeBegin = 0.3f; 
        ParticleTemplate.SizeVariation = 0.1f;
        ParticleTemplate.SizeEnd = 0.5f;   // szybko roœnie - rozprasza siê

        ParticleTemplate.LifeTime = 0.4f;  
        EmitRate = 0.1f;
    }
};
