#version 420 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D sceneColor;   // Oryginalna scena
uniform sampler2D bloomBlur;    // Rozmyte światło
// uniform sampler2D lutTexture; // Do podpięcia Twojej tekstury LUT 256x16 w przyszłości

uniform float exposure;         // np. 1.2
uniform float bloomStrength;    // np. 0.04 (dla cozy look)

void main()
{             
    vec3 hdrColor   = texture(sceneColor, TexCoords).rgb;      
    vec3 bloomColor = texture(bloomBlur,  TexCoords).rgb;
    
    // 1. Złożenie obrazu (Dodanie Blooma)
    hdrColor += bloomColor * bloomStrength; 
    
    // 2. Tone Mapping (ACES / Exposure) - zapobiega "przepaleniu" kolorów
    vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
    
    // 3. COLOR GRADING (Wersja proceduralna, by od razu nadać "Cozy" look)
    // Zwiększamy saturację (nasycenie barw)
    const vec3 W = vec3(0.2125, 0.7154, 0.0721);
    vec3 intensity = vec3(dot(result, W));
    result = mix(intensity, result, 1.2); // 1.2 = więcej kolorów
    
    // Ocieplamy obraz (więcej czerwieni/żółci, mniej niebieskiego)
    result *= vec3(1.02, 1.00, 0.97); // FIX: bylo (1.05,1.02,0.95) — za mocne rozjasnienie

    // FIX: było vec4(bloomColor, 1.0) — to był kod debugowy, który wyświetlał tylko bloom
    FragColor = vec4(result, 1.0);
}
