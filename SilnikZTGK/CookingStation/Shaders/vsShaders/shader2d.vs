#version 420 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 v_TexCoord;
out vec2 v_LocalPos; // LOKALNA POZYCJA (od 0.0 do 1.0) do liczenia zaokr�gle�

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;
uniform vec2 u_UVMin;
uniform vec2 u_UVMax;

void main() {
    v_TexCoord = u_UVMin + aTexCoord * (u_UVMax - u_UVMin);
    v_LocalPos = aPos.xy; // Przekazujemy pozycj� czystego quada w d� do Fragment Shadera
    gl_Position = u_ViewProjection * u_Transform * vec4(aPos, 1.0);
}