#version 420 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in float v_uvOffset;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_diffuse2;
uniform bool useTexture2;

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
    // 1. Animacja UV
    vec2 scrolledUV = vec2(TexCoords.x, TexCoords.y - v_uvOffset);
    vec3 baseColor = texture(texture_diffuse1, scrolledUV).rgb;

    // 2. Oświetlenie kierunkowe
    float ambientStrength = 0.55;
    vec3 ambient = ambientStrength * vec3(1.0);
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-u_SunDir);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = (diff * 0.45) * u_LightColor;

    float specularStrength = 0.5;
    int shininess = 16;
    vec3 viewDir = normalize(u_ViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * u_LightColor;

    vec3 result = (ambient + diffuse) * baseColor + specular;

    if (useTexture2) {
        vec3 emissiveColor = texture(texture_diffuse2, TexCoords).rgb;
        result += emissiveColor;
    }

    float gammaParam = 1.4;
    result = pow(result, vec3(1.0 / gammaParam));
    FragColor = vec4(result, 1.0);
}
