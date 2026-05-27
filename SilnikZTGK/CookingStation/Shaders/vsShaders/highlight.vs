#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

void main()
{
    // Minimalnie podnosimy vertex w osi Y w samym shaderze, 
    // aby unikn¹æ Z-Fightingu (przenikania tekstur) z pod³og¹
    vec3 liftedPos = aPos;
    liftedPos.y += 0.05; 

    gl_Position = u_ViewProjection * u_Transform * vec4(liftedPos, 1.0);
}