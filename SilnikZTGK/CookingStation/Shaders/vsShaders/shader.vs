#version 330 core

// 1. STANDARDOWE ATRYBUTY
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec2 aTexCoords2;

// 2. NOWE ATRYBUTY ASSIMP
layout (location = 4) in vec3 aTangent;
layout (location = 5) in vec3 aBitangent;

// 3. ANIMACJA SZKIELETOWA (ivec4 dla Int4!)
layout (location = 6) in ivec4 aBoneIDs; 
layout (location = 7) in vec4 aWeights;

// 4. INSTANCJONOWANIE 
layout (location = 8) in mat4 aInstanceMatrix; // zajmuje 8, 9, 10, 11
layout (location = 12) in float a_uvOffset;

out float v_uvOffset;
out vec2 TexCoords;
out vec2 TexCoords2; 
out vec3 Normal;
out vec3 FragPos;

uniform mat4 viewProjection;

// UNIFORMY ANIMACJI
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
        float totalWeight = 0.0; // Zmienna licz¹ca ³¹czn¹ wagê

        for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
        {
            if(aBoneIDs[i] == -1) 
                continue;
                
            if(aBoneIDs[i] >= MAX_BONES) 
                continue;

            mat4 boneTransform = finalBonesMatrices[aBoneIDs[i]];
            vec4 localPosition = boneTransform * vec4(aPos, 1.0);
            
            totalPosition += localPosition * aWeights[i];
            
            vec3 localNormal = mat3(boneTransform) * aNormal;
            totalNormal += localNormal * aWeights[i];

            // Dodajemy wagê, aby wiedzieæ, czy cokolwiek ruszy³o ten wierzcho³ek
            totalWeight += aWeights[i]; 
        }

        // Jeœli ³¹czna waga wynosi zero (b³¹d malowania wag w Blenderze),
        // shader u¿ywa domyœlnej pozycji wierzcho³ka
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
    gl_Position = viewProjection * aInstanceMatrix * totalPosition;
}