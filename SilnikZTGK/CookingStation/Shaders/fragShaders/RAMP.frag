#version 420 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1;
uniform sampler2D rampTex; // Tekstura Ramp (256x19), slot 10

layout (std140, binding = 0) uniform SceneData {
    mat4 u_ViewProjection;
    vec3 u_SunDir;
    float _pad0;
    vec3 u_LightColor;
    float _pad1;
    vec3 u_ViewPos;
    float _pad2;
};

void main() {
    vec3 baseColor = texture(texture_diffuse1, TexCoords).rgb;
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-u_SunDir);

    float diff = dot(norm, lightDir);
    float rampIndex = diff * 0.5 + 0.5;
    vec3 rampLight = texture(rampTex, vec2(rampIndex, 0.5)).rgb;

    vec3 result = baseColor * rampLight * u_LightColor;
    FragColor = vec4(result, 1.0);
}
