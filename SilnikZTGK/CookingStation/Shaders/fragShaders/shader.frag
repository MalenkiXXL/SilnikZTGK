#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec2 TexCoords2; 
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1; // Baza
uniform sampler2D texture_diffuse2; // Detale (Emissive)
uniform bool useTexture2; 

uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{    
    // 1. Pobieramy tylko kolor bazy (paleta)
    vec3 baseColor = texture(texture_diffuse1, TexCoords).rgb;

    // 2. Obliczenia œwiat³a (dzia³aj¹ tylko na bazowym kolorze)
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    float specularStrength = 0.3;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  
        
    // 3. Wynik samego oœwietlenia
    vec3 result = (ambient + diffuse + specular) * baseColor;

    // 4. DODAJEMY EMISSIVE NA KOÑCU
    if(useTexture2) {
        vec3 emissiveColor = texture(texture_diffuse2, TexCoords2).rgb;
        
        // Zwyk³e dodawanie wektorów!
        // T³o jest czarne (0,0,0), wiêc result + 0 = brak zmian.
        // Gdzie jest buŸka, tam dodajemy jej kolor do modelu.
        result += emissiveColor;
    }

    FragColor = vec4(result, 1.0);
}