#pragma once
#include "ParticleEmitterScript.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"

class SteamEmitterScript : public ParticleEmitterScript
{
public:
	void OnCreate() override
	{
		ParticleEmitterScript::OnCreate();

		//ladujemy tekstury
		ParticleTemplate.Textures.push_back(AssetManager::GetTexture2D("assets://particles/PotParticle.png"));
		ParticleTemplate.Textures.push_back(AssetManager::GetTexture2D("assets://particles/PotParticle2.png"));
		ParticleTemplate.Textures.push_back(AssetManager::GetTexture2D("assets://particles/PotParticle3.png"));

		//parametry pary wodnej
		ParticleTemplate.PositionOffset = { 0.0f, 0.4f, 0.5f };

		ParticleTemplate.ColorBegin = { 1.0f, 1.0f, 1.0f, 1.0f };
		ParticleTemplate.ColorEnd = { 1.0f, 1.0f, 1.0f, 0.8f };

		ParticleTemplate.SizeBegin = 0.7f;
		ParticleTemplate.SizeEnd = 0.5f;
		ParticleTemplate.SizeVariation = 0.3f;

		ParticleTemplate.Velocity = { 0.0f, 0.8f, 0.0f };
		ParticleTemplate.VelocityVariation = { 0.4f, 0.2f, 0.4f };

		ParticleTemplate.LifeTime = 2.0f;
		EmitRate = 0.1f;

		IsEmitting = false;

	}
};