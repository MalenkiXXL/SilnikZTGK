#pragma once
#include <vector>
#include <map>
#include <string>
#include <glm/glm.hpp>
#include <assimp/scene.h> 
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <spdlog/spdlog.h> // Wymagane do logowania b��d�w

#include "Bone.h"
#include "CookingStation/Renderer/Model.h" 

// Struktura na zbuforowane drzewo modelu
// ZABEZPIECZENIE: Zawsze inicjalizujemy warto�ci domy�lne na wypadek b��du �adowania!
struct AssimpNodeData
{
    glm::mat4 transformation = glm::mat4(1.0f);
    std::string name = "";
    int childrenCount = 0;
    std::vector<AssimpNodeData> children;
};

class Animation
{
public:
    Animation() = default;

    Animation(const std::string& animationPath, Model* model)
    {
        m_Path = animationPath;
        Assimp::Importer importer;
        importer.SetIOHandler(new VfsIOSystem()); 
        const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);

        // 1. Sprawdzamy czy plik w og�le istnieje i za�adowa� si� poprawnie
        if (!scene || !scene->mRootNode)
        {
            spdlog::error("Animator BLAD: Nie udalo sie wczytac pliku animacji! Sciezka: {}", animationPath);
            return;
        }

        // 2. Sprawdzamy, czy plik posiada jakiekolwiek klatki kluczowe
        if (scene->HasAnimations())
        {
            auto animation = scene->mAnimations[0];
            m_Duration = (float)animation->mDuration;
            m_TicksPerSecond = animation->mTicksPerSecond != 0 ? (float)animation->mTicksPerSecond : 25.0f;

            ReadHierarchyData(m_RootNode, scene->mRootNode);
            ReadMissingBones(animation, *model);
        }
       /* else
        {
            spdlog::warn("Animator OSTRZEZENIE: Plik wczytany poprawnie, ale NIE ZNALEZIONO ZADNEJ ANIMACJI: {}", animationPath);
        }*/
    }

    Bone* FindBone(const std::string& name)
    {
        auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
            [&](const Bone& b) { return b.GetBoneName() == name; }
        );
        if (iter == m_Bones.end()) return nullptr;
        else return &(*iter);
    }

    float GetTicksPerSecond() const { return m_TicksPerSecond; }
    float GetDuration() const { return m_Duration; }
    const std::string& GetPath() const { return m_Path; }
    const AssimpNodeData& GetRootNode() const { return m_RootNode; }
    const std::map<std::string, BoneInfo>& GetBoneIDMap() const { return m_BoneInfoMap; }

private:
    float m_Duration = 0.0f;
    float m_TicksPerSecond = 0.0f;
    std::string m_Path = "";
    std::vector<Bone> m_Bones;
    AssimpNodeData m_RootNode;
    std::map<std::string, BoneInfo> m_BoneInfoMap;

    void ReadMissingBones(const aiAnimation* animation, Model& model)
    {
        int size = animation->mNumChannels;
        auto& boneInfoMap = model.GetBoneInfoMap();
        int& boneCount = model.m_BoneCounter;

        for (int i = 0; i < size; i++)
        {
            auto channel = animation->mChannels[i];
            std::string boneName = channel->mNodeName.data;

            // Upewniamy si�, �e ko�� ma wygenerowane ID w modelu
            if (boneInfoMap.find(boneName) == boneInfoMap.end())
            {
                boneInfoMap[boneName].id = boneCount;
                boneCount++;
            }
            // Zapisujemy kluczow� struktur� ko�ci z jej klatkami 
            m_Bones.push_back(Bone(boneName, boneInfoMap[boneName].id, channel));
        }

        m_BoneInfoMap = boneInfoMap; // Kopiujemy gotowy s�ownik do instancji animacji
    }

    void ReadHierarchyData(AssimpNodeData& dest, const aiNode* src)
    {
        dest.name = src->mName.data;
        dest.transformation = AssimpMatToGLM(src->mTransformation); // Uzywamy tej samej konwersji co w Model.h
        dest.childrenCount = src->mNumChildren;

        for (int i = 0; i < src->mNumChildren; i++)
        {
            AssimpNodeData newData;
            ReadHierarchyData(newData, src->mChildren[i]);
            dest.children.push_back(newData);
        }
    }
};