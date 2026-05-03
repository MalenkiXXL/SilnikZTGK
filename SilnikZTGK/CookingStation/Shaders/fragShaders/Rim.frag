#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);

    // Dajemy lekki ambient, ¿eby model nie by³ ca³kowicie czarny z przodu
    vec3 baseColor = texture(texture_diffuse1, TexCoords).rgb;
    vec3 ambient = 0.2 * lightColor;

    // Rim Lighting - Wyk³adnik potêgi okreœlaj¹cy rozmycie otoczki
    float n_r = 3.0; 
    
    // Obliczamy "krawêdzie" (1 - cos(N, V))
    float rimFactor = pow(1.0 - max(dot(N, V), 0.0), n_r);
    
    // Dodajemy poœwiatê
    vec3 rim = rimFactor * lightColor; 

    // Wynik to delikatna baza + mocne podœwietlenie krawêdzi
    vec3 result = (baseColor * ambient) + rim;
    FragColor = vec4(result, 1.0);
}