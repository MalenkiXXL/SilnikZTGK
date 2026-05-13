#pragma once
#include <vector>
#include <assimp/scene.h>
#define GLM_ENABLE_EXPERIMENTAL 
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>

// Pomocnicze struktury dla klatek kluczowych
struct KeyPosition { glm::vec3 position; float timeStamp; };
struct KeyRotation { glm::quat orientation; float timeStamp; };
struct KeyScale { glm::vec3 scale; float timeStamp; };

class Bone
{
public:
    Bone(const std::string& name, int ID, const aiNodeAnim* channel)
        : m_Name(name), m_ID(ID), m_LocalTransform(1.0f)
    {
        // Kopiowanie pozycji (Translation)
        m_NumPositions = channel->mNumPositionKeys;
        for (int positionIndex = 0; positionIndex < m_NumPositions; ++positionIndex) {
            aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
            float timeStamp = channel->mPositionKeys[positionIndex].mTime;
            m_Positions.push_back({ glm::vec3(aiPosition.x, aiPosition.y, aiPosition.z), timeStamp });
        }

        // Kopiowanie rotacji (Rotation - kwaterniony!)
        m_NumRotations = channel->mNumRotationKeys;
        for (int rotationIndex = 0; rotationIndex < m_NumRotations; ++rotationIndex) {
            aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
            float timeStamp = channel->mRotationKeys[rotationIndex].mTime;
            m_Rotations.push_back({ glm::quat(aiOrientation.w, aiOrientation.x, aiOrientation.y, aiOrientation.z), timeStamp });
        }

        // Kopiowanie skali (Scale)
        m_NumScales = channel->mNumScalingKeys;
        for (int keyIndex = 0; keyIndex < m_NumScales; ++keyIndex) {
            aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
            float timeStamp = channel->mScalingKeys[keyIndex].mTime;
            m_Scales.push_back({ glm::vec3(scale.x, scale.y, scale.z), timeStamp });
        }
    }

    void Update(float animationTime)
    {
        glm::mat4 translation = InterpolatePosition(animationTime);
        glm::mat4 rotation = InterpolateRotation(animationTime);
        glm::mat4 scale = InterpolateScaling(animationTime);

        // Zbudowanie lokalnej transformacji dla danej klatki
        m_LocalTransform = translation * rotation * scale;
    }

    glm::mat4 GetLocalTransform() const { return m_LocalTransform; }
    std::string GetBoneName() const { return m_Name; }
    int GetBoneID() const { return m_ID; }

private:
    std::vector<KeyPosition> m_Positions;
    std::vector<KeyRotation> m_Rotations;
    std::vector<KeyScale> m_Scales;
    int m_NumPositions;
    int m_NumRotations;
    int m_NumScales;

    glm::mat4 m_LocalTransform;
    std::string m_Name;
    int m_ID;

    // Pobiera indeks klatki kluczowej bezposrednio przed aktualnym czasem
    int GetPositionIndex(float animationTime) {
        for (int index = 0; index < m_NumPositions - 1; ++index)
            if (animationTime < m_Positions[index + 1].timeStamp) return index;
        return 0;
    }

    int GetRotationIndex(float animationTime) {
        for (int index = 0; index < m_NumRotations - 1; ++index)
            if (animationTime < m_Rotations[index + 1].timeStamp) return index;
        return 0;
    }

    int GetScaleIndex(float animationTime) {
        for (int index = 0; index < m_NumScales - 1; ++index)
            if (animationTime < m_Scales[index + 1].timeStamp) return index;
        return 0;
    }

    // Funkcje interpoluj¹ce
    glm::mat4 InterpolatePosition(float animationTime) {
        if (1 == m_NumPositions) return glm::translate(glm::mat4(1.0f), m_Positions[0].position);
        int p0Index = GetPositionIndex(animationTime);
        int p1Index = p0Index + 1;
        float scaleFactor = GetScaleFactor(m_Positions[p0Index].timeStamp, m_Positions[p1Index].timeStamp, animationTime);
        glm::vec3 finalPosition = glm::mix(m_Positions[p0Index].position, m_Positions[p1Index].position, scaleFactor);
        return glm::translate(glm::mat4(1.0f), finalPosition);
    }

    glm::mat4 InterpolateRotation(float animationTime) {
        if (1 == m_NumRotations) return glm::toMat4(glm::normalize(m_Rotations[0].orientation));
        int p0Index = GetRotationIndex(animationTime);
        int p1Index = p0Index + 1;
        float scaleFactor = GetScaleFactor(m_Rotations[p0Index].timeStamp, m_Rotations[p1Index].timeStamp, animationTime);
        glm::quat finalRotation = glm::slerp(m_Rotations[p0Index].orientation, m_Rotations[p1Index].orientation, scaleFactor);
        finalRotation = glm::normalize(finalRotation);
        return glm::toMat4(finalRotation);
    }

    glm::mat4 InterpolateScaling(float animationTime) {
        if (1 == m_NumScales) return glm::scale(glm::mat4(1.0f), m_Scales[0].scale);
        int p0Index = GetScaleIndex(animationTime);
        int p1Index = p0Index + 1;
        float scaleFactor = GetScaleFactor(m_Scales[p0Index].timeStamp, m_Scales[p1Index].timeStamp, animationTime);
        glm::vec3 finalScale = glm::mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale, scaleFactor);
        return glm::scale(glm::mat4(1.0f), finalScale);
    }

    // Liczy wspó³czynnik interpolacji (0.0 do 1.0) miêdzy dwiema klatkami
    float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) {
        float scaleFactor = 0.0f;
        float midWayLength = animationTime - lastTimeStamp;
        float framesDiff = nextTimeStamp - lastTimeStamp;
        scaleFactor = midWayLength / framesDiff;
        return scaleFactor;
    }
};