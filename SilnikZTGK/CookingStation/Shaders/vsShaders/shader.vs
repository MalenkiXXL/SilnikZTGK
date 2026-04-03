#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec2 aTexCoords2; // DODANO: Odbiór danych z layoutu w Mesh.h

out vec2 TexCoords;
out vec2 TexCoords2; // DODANO: Przekazanie do Fragment Shadera

out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 viewProjection;

void main()
{
    TexCoords = aTexCoords;    
    TexCoords2 = aTexCoords2; // PRZYPISANIE: Przekazujemy UV2 dalej
    
    // Zabezpieczenie normalnych przy skalowaniu/obracaniu modelu
    Normal = mat3(transpose(inverse(model))) * aNormal; 
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    gl_Position = viewProjection * model * vec4(aPos, 1.0);
}