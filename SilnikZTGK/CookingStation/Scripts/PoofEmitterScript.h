#pragma once
#include "CookingStation/Scripts/ParticleEmitterScript.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"

class PoofEmitterScript : public ParticleEmitterScript
{
public:
    void OnCreate() override
    {
        ParticleEmitterScript::OnCreate();
        ParticleTemplate.Textures.push_back(AssetManager::GetTexture2D("assets://particles/PotParticle.png"));

        ParticleTemplate.PositionOffset = { 0.0f, -1.0f, 1.0f };

        ParticleTemplate.Velocity = { 0.0f, 0.15f, 0.0f };

        ParticleTemplate.VelocityVariation = { 1.6f, 0.0f, 1.6f };

        ParticleTemplate.ColorBegin = { 1.0f, 1.0f, 1.0f, 0.6f };
        ParticleTemplate.ColorEnd   = { 1.0f, 1.0f, 1.0f, 0.0f };

        ParticleTemplate.SizeBegin = 1.5f;
        ParticleTemplate.SizeVariation = 0.0f;

        ParticleTemplate.SizeEnd = 2.8f;

        ParticleTemplate.LifeTime = 1.4f;
        EmitRate = 0.05f;
    }
};