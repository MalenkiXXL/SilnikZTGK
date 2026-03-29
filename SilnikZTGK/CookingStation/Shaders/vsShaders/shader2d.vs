//ten shader tworzy kolorowy prostok¹t w miejscu wskazanym przez macierz

#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 v_TexCoord;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

uniform vec2 u_UVMin;
uniform vec2 u_UVMax;

void main() {
	v_TexCoord = u_UVMin + aTexCoord * (u_UVMax - u_UVMin);

	gl_Position = u_ViewProjection * u_Transform * vec4(aPos, 1.0);
}