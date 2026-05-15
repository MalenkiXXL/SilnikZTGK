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
    vec3 baseColor = texture(texture_diffuse1, TexCoords).rgb;
    vec3 N = normalize(Normal);
    vec3 L = normalize(-u_SunDir);
    vec3 V = normalize(u_ViewPos - FragPos);
    vec3 H = normalize(L + V);

    // 1. Wrapped Diffuse (Fake Subsurface Scattering)
    float NdotL = dot(N, L);
    float wrap = 0.5;
    float diffuseFactor = max((NdotL + wrap) / (1.0 + wrap), 0.0);
    diffuseFactor = pow(diffuseFactor, 2.0);
    vec3 diffuse = diffuseFactor * u_LightColor;

    // 2. Specular (Stylizowany Blinn-Phong)
    float NdotH = max(dot(N, H), 0.0);
    float specMask = smoothstep(0.0, 0.2, NdotL);
    float specularFactor = pow(NdotH, 32.0) * specMask;
    vec3 specular = specularFactor * u_LightColor * 0.5;

    // 3. Rim Lighting
    float NdotV = max(dot(N, V), 0.0);
    float rimFactor = 1.0 - NdotV;
    rimFactor = smoothstep(0.6, 1.0, rimFactor) * max(NdotL, 0.0);
    vec3 rimLight = rimFactor * u_LightColor * vec3(1.0, 1.0, 1.0);

    // Ambient
    vec3 ambient = 0.15 * u_LightColor;

    vec3 result = baseColor * (ambient + diffuse) + specular + rimLight;
    FragColor = vec4(result, 1.0);
}
