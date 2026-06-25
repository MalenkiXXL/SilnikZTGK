#version 420 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1;

layout (std140, binding = 0) uniform SceneData {
    mat4 u_ViewProjection;
    mat4 u_LightSpaceMatrix;
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
    vec3 L = normalize(-u_SunDir);
    vec3 V = normalize(u_ViewPos - FragPos);
    vec3 H = normalize(L + V);

    vec3 ambient = 0.15 * u_LightColor;

    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * u_LightColor;

    float shininess = 32.0;
    float spec = pow(max(dot(N, H), 0.0), shininess);
    vec3 specular = 0.5 * spec * u_LightColor;

    vec3 baseColor = texture(texture_diffuse1, TexCoords).rgb;
    vec3 result = baseColor * (ambient + diffuse) + specular;
    FragColor = vec4(result, 1.0);
}
