#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1; // Podstawowa tekstura modelu (np. kolor drewna)
uniform sampler2D rampTex;          // Tekstura Ramp (256x19)

uniform vec3 lightColor;
uniform vec3 lightPos;

void main() {
    vec3 baseColor = texture(texture_diffuse1, TexCoords).rgb;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    // Obliczamy oœwietlenie (-1.0 do 1.0) i przesuwamy na (0.0 do 1.0)
    float diff = dot(norm, lightDir);
    float rampIndex = diff * 0.5 + 0.5;

    // Pobieramy kolor z tekstury Ramp. Y ustalamy na 0.5 (œrodek tekstury o wys. 19px)
    vec3 rampLight = texture(rampTex, vec2(rampIndex, 0.5)).rgb;

    // Mno¿ymy kolor bazy przez pobrany z rampy gradient i kolor œwiat³a
    vec3 result = baseColor * rampLight * lightColor;

    FragColor = vec4(result, 1.0);
}