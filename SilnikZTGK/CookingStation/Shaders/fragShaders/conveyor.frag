#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1; // cube (belt.png)
uniform sampler2D texture_diffuse2; // plane (a.png)
uniform bool useTexture2;

in float v_uvOffset;

uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    // 1. OBLICZANIE PRZESUNIÊCIA UV
    // Znak MINUS zmienia kierunek. Mno¿nik (np. 0.3) spowalnia animacjê.
    // U¿ycie funkcji fract() to bardzo dobra praktyka, zapobiega b³êdom precyzji!
    //float speedMultiplier = 0.3; // Zmieniaj tê wartoœæ, by dopasowaæ prêdkoœæ
    vec2 scrolledUV = vec2(TexCoords.x, TexCoords.y - v_uvOffset);
    //vec2 scrolledUV = vec2(TexCoords.x, fract(TexCoords.y - (v_uvOffset * speedMultiplier)));

    // 2. POBRANIE KOLORU (teraz zawsze u¿ywamy przesuniêtych kordów)
    vec3 baseColor = texture(texture_diffuse1, scrolledUV).rgb;

    // 3. OŒWIETLENIE
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

    // 4. WYNIK KOÑCOWY (Œwiat³o * Przesuwaj¹ca siê tekstura)
    vec3 result = (ambient + diffuse + specular) * baseColor;
    
    // Zwracamy oœwietlony piksel!
    FragColor = vec4(result, 1.0);
}