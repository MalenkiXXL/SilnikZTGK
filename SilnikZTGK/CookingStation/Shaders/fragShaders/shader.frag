#version 420 core
out vec4 FragColor;

// Dane wejściowe z Vertex Shadera (muszą pasować do out w VS)
in float v_uvOffset;
in vec2 TexCoords;
in vec2 TexCoords2; 
in vec3 Normal;
in vec3 FragPos;

layout (std140, binding = 0) uniform SceneData {
    mat4 u_ViewProjection;
    vec3 u_SunDir;
    float _pad0;
    vec3 u_LightColor;
    float _pad1;
    vec3 u_ViewPos;
    float _pad2;
};

// Tekstury i flagi (Per-Material, zostają jako zwykłe uniformy)
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_diffuse2;
uniform bool useTexture2; 

void main()
{    
    vec3 baseColor = texture(texture_diffuse1, TexCoords).rgb;

if(useTexture2) {
    vec4 tex2Sample = texture(texture_diffuse2, TexCoords2);
    float lum = max(tex2Sample.r, max(tex2Sample.g, tex2Sample.b));
    // Odtwórz prawdziwy kolor plamy (np. ciemnopomarańczowy na krawędzi → pełny pomarańczowy)
    vec3 trueColor = tex2Sample.rgb / max(lum, 0.001);
    // lum jako waga: 0 = tło czarne, 1 = środek plamy
    baseColor = mix(baseColor, trueColor, lum * tex2Sample.a);
}
    // Szachownica na podłodze (logika bez zmian)
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

    // Oświetlenie kierunkowe (Directional light)
    
    // Ambient
    float ambientStrength = 0.55; 
    vec3 ambient = ambientStrength * vec3(1.0); 

    // Diffuse - używamy u_SunDir i u_LightColor z UBO
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-u_SunDir); 
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = (diff * 0.45) * u_LightColor; 

    // Specular - używamy u_ViewPos i u_LightColor z UBO
    float specularStrength = 0.5; 
    int shininess = 16;           
    
    vec3 viewDir = normalize(u_ViewPos - FragPos); 
    vec3 reflectDir = reflect(-lightDir, norm);
    
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * u_LightColor;  

    // Złożenie oświetlenia
    vec3 result = (ambient + diffuse) * baseColor + specular;

    // Korekcja gamma
    float gammaParam = 1.4; 
    result = pow(result, vec3(1.0 / gammaParam)); 

    FragColor = vec4(result, 1.0);
}