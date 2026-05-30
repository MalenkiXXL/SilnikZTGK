#version 420 core

layout (std140, binding = 0) uniform SceneData {
    mat4 u_ViewProjection;
    vec3 u_SunDir;
    float _pad0;
    vec3 u_LightColor;
    float _pad1;
    vec3 u_ViewPos;
    float _pad2;
};

// 2. STANDARDOWE ATRYBUTY
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec2 aTexCoords2;

// 3. NOWE ATRYBUTY ASSIMP
layout (location = 4) in vec3 aTangent;
layout (location = 5) in vec3 aBitangent;

// 4. ANIMACJA SZKIELETOWA
layout (location = 6) in ivec4 aBoneIDs;
layout (location = 7) in vec4 aWeights;

// 5. INSTANCJONOWANIE 
layout (location = 8) in mat4 aInstanceMatrix;
// zajmuje 8, 9, 10, 11
layout (location = 12) in float a_uvOffset;

out float v_uvOffset;
out vec2 TexCoords;
out vec2 TexCoords2; 
out vec3 Normal;
out vec3 FragPos;

// POZOSTA£E UNIFORMY
const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];
uniform bool u_Animated;

void main()
{
    TexCoords = aTexCoords;
    TexCoords2 = aTexCoords2;
    v_uvOffset = a_uvOffset; 

    vec4 totalPosition = vec4(0.0);
    vec3 totalNormal = vec3(0.0);

    if (u_Animated)
    {
        float totalWeight = 0.0;
        for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
        {
            if(aBoneIDs[i] == -1) continue;
            if(aBoneIDs[i] >= MAX_BONES) continue;

            mat4 boneTransform = finalBonesMatrices[aBoneIDs[i]];
            totalPosition += (boneTransform * vec4(aPos, 1.0)) * aWeights[i];
            totalNormal += (mat3(boneTransform) * aNormal) * aWeights[i];
            totalWeight += aWeights[i];
        }

        if (totalWeight < 0.01) 
        {
            totalPosition = vec4(aPos, 1.0);
            totalNormal = aNormal;
        }
    }
    else
    {
        totalPosition = vec4(aPos, 1.0);
        totalNormal = aNormal;
    }

    Normal = mat3(transpose(inverse(aInstanceMatrix))) * totalNormal;
    FragPos = vec3(aInstanceMatrix * totalPosition);
    gl_Position = u_ViewProjection * aInstanceMatrix * totalPosition;
}