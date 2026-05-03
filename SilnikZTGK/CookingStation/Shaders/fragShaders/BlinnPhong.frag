#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    vec3 N = normalize(Normal);
    vec3 L = normalize(lightPos - FragPos);
    vec3 V = normalize(viewPos - FragPos);
    
    // Wektor po³ówkowy (Half-vector) z wyk³adu
    vec3 H = normalize(L + V); 

    // Ambient
    vec3 ambient = 0.15 * lightColor;

    // Diffuse
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular (Blinn-Phong) - liczymy cosinus miêdzy wektorem normalnym a po³ówkowym
    float shininess = 32.0;
    float spec = pow(max(dot(N, H), 0.0), shininess);
    vec3 specular = 0.5 * spec * lightColor;

    vec3 baseColor = texture(texture_diffuse1, TexCoords).rgb;
    
    // £¹czymy w ca³oœæ
    vec3 result = baseColor * (ambient + diffuse) + specular;
    FragColor = vec4(result, 1.0);
}