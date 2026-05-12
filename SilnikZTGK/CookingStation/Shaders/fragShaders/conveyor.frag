#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

// ODBIERAMY ATRYBUT Z VERTEX SHADERA (Zamiast uniformu!)
in float v_uvOffset;

uniform sampler2D texture_diffuse1; // cube (belt.png)
uniform sampler2D texture_diffuse2; // plane (a.png)
uniform bool useTexture2;

// Ujednolicone parametry œwiat³a z g³ównego shadera
uniform vec3 lightColor;
uniform vec3 sunDir; // Zast¹pi³o lightPos!
uniform vec3 viewPos;

void main()
{
    // 1. OBLICZANIE PRZESUNIÊCIA UV (Animacja)
    vec2 scrolledUV = vec2(TexCoords.x, TexCoords.y - v_uvOffset);

    // 2. POBRANIE KOLORU
    vec3 baseColor = texture(texture_diffuse1, scrolledUV).rgb;

    // 3. OŒWIETLENIE KIERUNKOWE (Identyczne jak w reszcie fabryki)
    float ambientStrength = 0.55;
    vec3 ambient = ambientStrength * vec3(1.0);

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-sunDir); // Œwiat³o s³oñca
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = (diff * 0.45) * lightColor;

    float specularStrength = 0.5;
    int shininess = 16;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse) * baseColor + specular;

    // Mo¿liwoœæ na³o¿enia drugiej tekstury (jak w Twoim oryginale)
    if(useTexture2) {
        vec3 emissiveColor = texture(texture_diffuse2, TexCoords).rgb;
        result += emissiveColor;
    }

    // 4. KOREKCJA GAMMA (Im wy¿sza, tym bardziej blade - ujednolicone na 1.4)
    float gammaParam = 1.4;
    result = pow(result, vec3(1.0 / gammaParam));

    FragColor = vec4(result, 1.0);
}