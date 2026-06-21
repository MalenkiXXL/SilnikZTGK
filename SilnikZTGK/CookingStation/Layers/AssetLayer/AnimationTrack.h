#pragma once
#include <vector>
#include <assimp/scene.h>
#define GLM_ENABLE_EXPERIMENTAL 
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>

class AnimationTrack
{
public:
    AnimationTrack(const std::string& name, const aiNodeAnim* channel)
        : m_Name(name), m_LocalTransform(1.0f)
    {
        m_NumPositions = channel->mNumPositionKeys;
        for (int positionIndex = 0; positionIndex < m_NumPositions; ++positionIndex) {
            aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
            float timeStamp = channel->mPositionKeys[positionIndex].mTime;
            m_Positions.push_back({ glm::vec3(aiPosition.x, aiPosition.y, aiPosition.z), timeStamp });
        }

        m_NumRotations = channel->mNumRotationKeys;
        for (int rotationIndex = 0; rotationIndex < m_NumRotations; ++rotationIndex) {
            aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
            float timeStamp = channel->mRotationKeys[rotationIndex].mTime;
            m_Rotations.push_back({ glm::quat(aiOrientation.w, aiOrientation.x, aiOrientation.y, aiOrientation.z), timeStamp });
        }

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

        m_LocalTransform = translation * rotation * scale;
    }

    glm::mat4 GetLocalTransform() const { return m_LocalTransform; }
    std::string GetBoneName() const { return m_Name; }
    int GetBoneID() const { return m_ID; }

    // Nowe metody zwracające wyliczone wektory dla danego czasu
    glm::vec3 GetInterpolatedPosition(float animationTime) {
        if (m_Positions.empty()) return glm::vec3(0.0f);
        if (1 == m_NumPositions) return m_Positions[0].position;
        int p0Index = GetPositionIndex(animationTime);
        int p1Index = p0Index + 1;
        float scaleFactor = GetScaleFactor(m_Positions[p0Index].timeStamp, m_Positions[p1Index].timeStamp, animationTime);
        return glm::mix(m_Positions[p0Index].position, m_Positions[p1Index].position, scaleFactor);
    }

    glm::vec3 GetInterpolatedRotationEuler(float animationTime) {
        if (m_Rotations.empty()) return glm::vec3(0.0f);
        if (1 == m_NumRotations) return glm::degrees(glm::eulerAngles(glm::normalize(m_Rotations[0].orientation)));
        int p0Index = GetRotationIndex(animationTime);
        int p1Index = p0Index + 1;
        float scaleFactor = GetScaleFactor(m_Rotations[p0Index].timeStamp, m_Rotations[p1Index].timeStamp, animationTime);
        glm::quat finalRotation = glm::slerp(m_Rotations[p0Index].orientation, m_Rotations[p1Index].orientation, scaleFactor);
        return glm::degrees(glm::eulerAngles(glm::normalize(finalRotation))); // Zwracamy stopnie dla Twojego TransformComponent
    }

    glm::vec3 GetInterpolatedScale(float animationTime) {
        if (m_Scales.empty()) return glm::vec3(1.0f);
        if (1 == m_NumScales) return m_Scales[0].scale;
        int p0Index = GetScaleIndex(animationTime);
        int p1Index = p0Index + 1;
        float scaleFactor = GetScaleFactor(m_Scales[p0Index].timeStamp, m_Scales[p1Index].timeStamp, animationTime);
        return glm::mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale, scaleFactor);
    }

    std::string GetTrackName() const { return m_Name; }


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

    int GetPositionIndex(float animationTime) {
        for (int index = 0; index < m_NumPositions - 1; ++index)
            if (animationTime < m_Positions[index + 1].timeStamp) return index;
        // ZMIANA: Zwracamy przedostatni indeks zamiast 0, gdy czas minął!
        return std::max(0, m_NumPositions - 2);
    }

    int GetRotationIndex(float animationTime) {
        for (int index = 0; index < m_NumRotations - 1; ++index)
            if (animationTime < m_Rotations[index + 1].timeStamp) return index;
        return std::max(0, m_NumRotations - 2);
    }

    int GetScaleIndex(float animationTime) {
        for (int index = 0; index < m_NumScales - 1; ++index)
            if (animationTime < m_Scales[index + 1].timeStamp) return index;
        return std::max(0, m_NumScales - 2);
    }

    float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) {
        float midWayLength = animationTime - lastTimeStamp;
        float framesDiff = nextTimeStamp - lastTimeStamp;
        if (framesDiff <= 0.0f) return 0.0f;
        float scaleFactor = midWayLength / framesDiff;
        return glm::clamp(scaleFactor, 0.0f, 1.0f);
    }

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

};