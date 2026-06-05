#version 420 core
out vec4 FragColor;

// Dane wejściowe z Vertex Shadera
in float v_uvOffset;
in vec2 TexCoords;
in vec2 TexCoords2; 
in vec3 Normal;
in vec3 FragPos;
in vec4 FragPosLightSpace; // DODANE

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

// Tekstury i flagi
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_diffuse2;
uniform bool useTexture2; 

uniform sampler2D shadowMap; // DODANE

// DODANE: Funkcja sprawdzająca cień
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
    vec3 baseColor = texture(texture_diffuse1, TexCoords).rgb;

    if(useTexture2) {
        vec4 tex2Sample = texture(texture_diffuse2, TexCoords2);
        float lum = max(tex2Sample.r, max(tex2Sample.g, tex2Sample.b));
        // Odtwórz prawdziwy kolor plamy
        vec3 trueColor = tex2Sample.rgb / max(lum, 0.001);
        baseColor = mix(baseColor, trueColor, lum * tex2Sample.a);
    }
    
    // Szachownica na podłodze 
    if (FragPos.y < 0.0) 
    {
        float gridSize = 2.0;
        float cellX = floor(FragPos.x / gridSize);
        float cellZ = floor(FragPos.z / gridSize);
        
        float checker = mod(cellX + cellZ, 2.0);
        if (checker > 0.5) 
        {
            baseColor *= 0.90; 
        }
    }

    // Oświetlenie kierunkowe
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

    // Obliczenie Cienia
    float shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);

    // ZMODYFIKOWANE Złożenie oświetlenia (uwzględnienie cienia)
    vec3 result = (ambient + (1.0 - shadow) * diffuse) * baseColor + (1.0 - shadow) * specular;
    
    // Korekcja gamma
    float gammaParam = 1.4; 
    result = pow(result, vec3(1.0 / gammaParam)); 

    FragColor = vec4(result, 1.0);
}