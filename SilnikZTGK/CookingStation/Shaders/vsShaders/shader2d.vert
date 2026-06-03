#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in float aTexIndex;
layout (location = 4) in vec2 aQuadSize;
layout (location = 5) in float aRadius;

uniform mat4 u_ViewProjection;

out vec4 v_Color;
out vec2 v_TexCoord;
out float v_TexIndex;
out vec2 v_QuadSize;
out float v_Radius;

void main() {
    v_Color = aColor;
    v_TexCoord = aTexCoord;
    v_TexIndex = aTexIndex;
    v_QuadSize = aQuadSize;
    v_Radius = aRadius;
    
    gl_Position = u_ViewProjection * vec4(aPos, 1.0);
}