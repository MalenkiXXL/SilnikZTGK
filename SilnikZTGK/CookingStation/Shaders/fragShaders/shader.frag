#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

// Assimp ³aduje tekstury jako "texture_diffuse1"
uniform sampler2D texture_diffuse1;

// Zmienne œwiat³a, które wysy³amy z main.cpp
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{    
    // Próbujemy pobraæ kolor z tekstury. Jeœli jej nie ma, bêdzie prawie czarny.
    // Dodajemy ma³y szary wektor (0.5), ¿eby model bez tekstury wygl¹da³ jak glina.
    vec3 texColor = texture(texture_diffuse1, TexCoords).rgb + vec3(0.5);

    // 1. Ambient (œwiat³o otoczenia)
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;
  	
    // 2. Diffuse (œwiat³o rozproszone)
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // 3. Specular (odblask)
    float specularStrength = 0.3;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  
        
    // £¹czymy œwiat³o z kolorem "gliny" lub tekstury
    vec3 result = (ambient + diffuse + specular) * texColor;
    FragColor = vec4(result, 1.0);
}