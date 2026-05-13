#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <cmath> 
#include <memory>
#include <unordered_map>
#include <string>
#include <spdlog/spdlog.h>
#include "CookingStation/Layers/AssetLayer/Animation.h"

class Animator
{
public:
    Animator() // Pusty konstruktor, animacje dodamy póŸniej
    {
        m_CurrentTime = 0.0f;
        m_CurrentAnimationName = "";
        m_FinalBoneMatrices.reserve(100);
        for (int i = 0; i < 100; i++)
            m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
    }

    // Dodaje now¹ kasetê do naszego s³ownika
    void AddAnimation(const std::string& name, std::shared_ptr<Animation> anim)
    {
        m_Animations[name] = anim;
        // Jeœli to pierwsza dodana animacja, ustawmy j¹ domyœlnie
        if (m_CurrentAnimation == nullptr) {
            PlayAnimation(name);
        }
    }

    void PlayAnimation(const std::string& name)
    {
        // BARDZO WA¯NE: Nie resetuj czasu, jeœli ta animacja JU¯ TERAZ leci
        if (m_CurrentAnimationName == name) return;

        if (m_Animations.find(name) != m_Animations.end())
        {
            m_CurrentAnimation = m_Animations[name];
            m_CurrentAnimationName = name;
            m_CurrentTime = 0.0f;
            spdlog::info("Animator: Przelaczono na animacje '{}'", name);
        }
        else
        {
            spdlog::warn("Animator B£¥D: Nie znaleziono animacji o nazwie '{}'!", name);
        }
    }

    void UpdateAnimation(float dt)
    {
        if (m_CurrentAnimation && m_CurrentAnimation->GetDuration() > 0.0f)
        {
            m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
            m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
            CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
        }
    }

    void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform)
    {
        std::string nodeName = node->name;
        glm::mat4 nodeTransform = node->transformation;

        Bone* currentBone = m_CurrentAnimation->FindBone(nodeName);
        if (currentBone)
        {
            currentBone->Update(m_CurrentTime);
            nodeTransform = currentBone->GetLocalTransform();
        }

        glm::mat4 globalTransformation = parentTransform * nodeTransform;
        const auto& boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
        auto it = boneInfoMap.find(nodeName);
        if (it != boneInfoMap.end())
        {
            int index = it->second.id;
            glm::mat4 offset = it->second.offset;
            m_FinalBoneMatrices[index] = globalTransformation * offset;
        }

        for (int i = 0; i < node->childrenCount; i++)
            CalculateBoneTransform(&node->children[i], globalTransformation);
    }

    const std::vector<glm::mat4>& GetFinalBoneMatrices() const {
        return m_FinalBoneMatrices;
    }

private:
    std::vector<glm::mat4> m_FinalBoneMatrices;

    std::unordered_map<std::string, std::shared_ptr<Animation>> m_Animations;
    std::shared_ptr<Animation> m_CurrentAnimation = nullptr;
    std::string m_CurrentAnimationName;
    float m_CurrentTime;
};