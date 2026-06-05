#version 420 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in float v_uvOffset;
in vec4 FragPosLightSpace; // DODANE

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_diffuse2;
uniform bool useTexture2;

uniform sampler2D shadowMap; // DODANE: Tekstura cienia bindowana na slot 15

layout (std140, binding = 0) uniform SceneData {
    mat4 u_ViewProjection;
    mat4 u_LightSpaceMatrix; // DODANE
    vec3 u_SunDir;
    float _pad0;
    vec3 u_LightColor;
    float _pad1;
    vec3 u_ViewPos;
    float _pad2;
};

// DODANE: Funkcja sprawdzająca czy piksel jest w cieniu z miękkimi krawędziami (PCF)
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0) return 0.0;
    
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    return shadow;
}

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

    // 3. Obliczenie Cienia
    float shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);

    // ZMODYFIKOWANE: Cień wygasza diffuse i specular, ale nie rusza ambientu
    vec3 result = (ambient + (1.0 - shadow) * diffuse) * baseColor + (1.0 - shadow) * specular;
    
    if (useTexture2) {
        vec3 emissiveColor = texture(texture_diffuse2, TexCoords).rgb;
        result += emissiveColor; // Elementy świecące (emissive) ignorują cień
    }

    // 4. Korekcja gamma
    float gammaParam = 1.4;
    result = pow(result, vec3(1.0 / gammaParam));
    
    FragColor = vec4(result, 1.0);
}