#version 420 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1;

layout (std140, binding = 0) uniform SceneData {
    mat4 u_ViewProjection;
    vec3 u_SunDir;
    float _pad0;
    vec3 u_LightColor;
    float _pad1;
    vec3 u_ViewPos;
    float _pad2;
};

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(u_ViewPos - FragPos);
    vec3 baseColor = texture(texture_diffuse1, TexCoords).rgb;

    vec3 ambient = 0.2 * u_LightColor;

    // Rim Lighting
    float n_r = 3.0;
    float rimFactor = pow(1.0 - max(dot(N, V), 0.0), n_r);
    vec3 rim = rimFactor * u_LightColor;

    vec3 result = (baseColor * ambient) + rim;
    FragColor = vec4(result, 1.0);
}
