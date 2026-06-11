#version 420 core

layout (std140, binding = 0) uniform SceneData {
    mat4 u_ViewProjection;
    mat4 u_LightSpaceMatrix; 
    vec3 u_SunDir;
    float _pad0;
    vec3 u_LightColor;
    float _pad1;
    vec3 u_ViewPos;
    float _pad2;
};

layout (location = 0) in vec3 aPos;

layout (location = 6) in ivec4 aBoneIDs;
layout (location = 7) in vec4 aWeights;
const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];
uniform bool u_Animated;

layout (location = 8) in mat4 aInstanceMatrix;

void main()
{
    vec4 totalPosition = vec4(0.0);
    
    if (u_Animated)
    {
        float totalWeight = 0.0;
        for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
        {
            if(aBoneIDs[i] == -1 || aBoneIDs[i] >= MAX_BONES) continue;
            mat4 boneTransform = finalBonesMatrices[aBoneIDs[i]];
            totalPosition += (boneTransform * vec4(aPos, 1.0)) * aWeights[i];
            totalWeight += aWeights[i];
        }
        if (totalWeight < 0.01) totalPosition = vec4(aPos, 1.0);
    }
    else
    {
        totalPosition = vec4(aPos, 1.0);
    }

    gl_Position = u_LightSpaceMatrix * aInstanceMatrix * totalPosition;
}