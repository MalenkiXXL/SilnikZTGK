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
    vec3 baseColor = texture(texture_diffuse1, TexCoords).rgb;

    vec3 N = normalize(Normal);
    vec3 L = normalize(lightPos - FragPos);
    vec3 V = normalize(viewPos - FragPos);
    vec3 H = normalize(L + V);

    // 1. Wrapped Diffuse (Fake Subsurface Scattering)
    // Zamiast uci¹æ cieñ przy 0, przesuwamy go (N*L + w) / (1 + w)
    float NdotL = dot(N, L);
    float wrap = 0.5; 
    float diffuseFactor = max((NdotL + wrap) / (1.0 + wrap), 0.0);
    // Podnosimy do potêgi, ¿eby odzyskaæ kontrast
    diffuseFactor = pow(diffuseFactor, 2.0); 
    vec3 diffuse = diffuseFactor * lightColor;

    // 2. Specular (Stylizowany Blinn-Phong)
    float NdotH = max(dot(N, H), 0.0);
    // Smoothstep maskuje b³ysk, by nie pojawia³ siê na krawêdzi cienia
    float specMask = smoothstep(0.0, 0.2, NdotL); 
    float specularFactor = pow(NdotH, 32.0) * specMask;
    vec3 specular = specularFactor * lightColor * 0.5; // Si³a b³ysku 0.5

    // 3. Rim Lighting (œwiat³o na krawêdziach obiektu)
    float NdotV = max(dot(N, V), 0.0);
    // Odwracamy (1 - N*V) ¿eby najmocniejsze by³o na brzegach
    float rimFactor = 1.0 - NdotV;
    // Ograniczamy rim, by pojawia³ siê tylko tam, gdzie pada œwiat³o
    rimFactor = smoothstep(0.6, 1.0, rimFactor) * max(NdotL, 0.0);
    vec3 rimLight = rimFactor * lightColor * vec3(1.0, 1.0, 1.0); // Bia³a poœwiata na krawêdzi

    // Ambient
    vec3 ambient = 0.15 * lightColor;

    vec3 result = baseColor * (ambient + diffuse) + specular + rimLight;
    
    FragColor = vec4(result, 1.0);
}