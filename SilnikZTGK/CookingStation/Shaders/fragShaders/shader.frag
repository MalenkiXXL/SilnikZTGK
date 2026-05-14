#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec2 TexCoords2; 
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_diffuse2;
uniform bool useTexture2; 

uniform vec3 lightColor;
uniform vec3 sunDir;      
uniform vec3 viewPos;

void main()
{    
    vec3 baseColor = texture(texture_diffuse1, TexCoords).rgb;

    // Szachownica na pod³odze
    if (FragPos.y < 0.0) 
    {
        float gridSize = 2.0; 
        float cellX = floor(FragPos.x / gridSize);
        float cellZ = floor(FragPos.z / gridSize);
        
        float checker = mod(cellX + cellZ, 2.0);
        
        if (checker > 0.5) 
        {
            baseColor *= 0.90; // Jasnoœæ ciemniejszych pól -> wiêksza liczba to jaœniejsze 
        }
    }

   // Directional light z korekcj¹
   // Ambient
    float ambientStrength = 0.55; 
    vec3 ambient = ambientStrength * vec3(1.0); 

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-sunDir); 
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = (diff * 0.45) * lightColor; 

    // Specular
    float specularStrength = 0.5; // Si³a odblasku 
    int shininess = 16;           // "Ostroœæ" odblasku
    
    vec3 viewDir = normalize(viewPos - FragPos); 
    vec3 reflectDir = reflect(-lightDir, norm);
    
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;  

    // Z³o¿enie œwiat³a
    vec3 result = (ambient + diffuse) * baseColor + specular;

    if(useTexture2) {
        vec3 emissiveColor = texture(texture_diffuse2, TexCoords2).rgb;
        result += emissiveColor;
    }

    // Korekcja gamma -> Im wiêksza tym bardziej blade
    float gammaParam = 1.4; 
    result = pow(result, vec3(1.0 / gammaParam)); 

    FragColor = vec4(result, 1.0);
}