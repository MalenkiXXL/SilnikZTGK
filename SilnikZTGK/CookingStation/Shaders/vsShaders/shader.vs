#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec2 aTexCoords2;

layout (location = 8) in mat4 aInstanceMatrix;

out vec2 TexCoords;
out vec2 TexCoords2; 

out vec3 Normal;
out vec3 FragPos;

uniform mat4 viewProjection;

void main()
{
    TexCoords = aTexCoords;    
    TexCoords2 = aTexCoords2;

    // Poprawne rzutowanie normalnych
    Normal = mat3(transpose(inverse(aInstanceMatrix))) * aNormal;
    
    FragPos = vec3(aInstanceMatrix * vec4(aPos, 1.0));
    gl_Position = viewProjection * aInstanceMatrix * vec4(aPos, 1.0);
}