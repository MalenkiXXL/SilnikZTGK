#pragma once
#include <unordered_map>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "AnimationTrack.h"
#include <spdlog/spdlog.h>

class NodeAnimation
{
public:
    NodeAnimation(std::string animationPath)
    {
        const std::string prefix = "CookingStation/Assets/";
        if (animationPath.find(prefix) == 0) {
            animationPath = "assets://" + animationPath.substr(prefix.length());
        }

        Assimp::Importer importer;

        importer.SetIOHandler(new VfsIOSystem());

        const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);

        if (!scene || !scene->mRootNode || !scene->HasAnimations()) {
            spdlog::error("NodeAnimation: Nie znaleziono animacji lub pliku: {}", animationPath);
            return;
        }

        auto animation = scene->mAnimations[0];
        m_Duration = (float)animation->mDuration;
        m_TicksPerSecond = animation->mTicksPerSecond != 0 ? (float)animation->mTicksPerSecond : 25.0f;

        for (unsigned int i = 0; i < animation->mNumChannels; i++)
        {
            auto channel = animation->mChannels[i];
            std::string nodeName = channel->mNodeName.data;
            m_Tracks.emplace(nodeName, AnimationTrack(nodeName, channel));
        }
    }
    float GetDuration() const { return m_Duration; }
    float GetTicksPerSecond() const { return m_TicksPerSecond; }
    AnimationTrack* GetTrack(const std::string& name) {
        auto it = m_Tracks.find(name);
        return (it != m_Tracks.end()) ? &it->second : nullptr;
    }
    const std::unordered_map<std::string, AnimationTrack>& GetTracks() const { return m_Tracks; }

private:
    float m_Duration = 0.0f;
    float m_TicksPerSecond = 0.0f;
    std::unordered_map<std::string, AnimationTrack> m_Tracks;
};