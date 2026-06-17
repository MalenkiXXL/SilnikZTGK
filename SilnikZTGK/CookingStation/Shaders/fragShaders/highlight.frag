#version 420 core
out vec4 FragColor;

in float v_uvOffset;
in vec2 TexCoords;
in vec2 TexCoords2; 
in vec3 Normal;
in vec3 FragPos;
in vec4 v_HighlightColor;

layout (std140, binding = 0) uniform SceneData {
    mat4 u_ViewProjection;
    mat4 u_LightSpaceMatrix;
    vec3 u_SunDir;
    float _pad0;
    vec3 u_LightColor;
    float _pad1;
    vec3 u_ViewPos;
    float _pad2;
};

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_diffuse2;
uniform bool useTexture2; 

void main()
{
    vec4 texColor = texture(texture_diffuse1, TexCoords);
    float tintOpacity = v_HighlightColor.a;

    vec3 baseColor = texColor.rgb;

    float ambientStrength = 0.55;
    vec3 ambient = ambientStrength * vec3(1.0); 

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-u_SunDir); 
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = (diff * 0.45) * u_LightColor;

    float specularStrength = 0.5; 
    int shininess = 16;
    vec3 viewDir = normalize(u_ViewPos - FragPos); 
    vec3 reflectDir = reflect(-lightDir, norm);
    
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * u_LightColor;  

    vec3 result = (ambient + diffuse) * baseColor + specular;

    vec3 glowColor = v_HighlightColor.rgb * 1.2;
    result = mix(result, glowColor, tintOpacity);

    float rimFactor = 1.0 - max(dot(viewDir, norm), 0.0);
    rimFactor = smoothstep(0.5, 1.0, rimFactor);
    vec3 rimGlow = v_HighlightColor.rgb * rimFactor * tintOpacity * 1.5;
    result += rimGlow;

    if(useTexture2) {
        vec3 emissiveColor = texture(texture_diffuse2, TexCoords2).rgb;
        result += emissiveColor;
    }

    float gammaParam = 1.4;
    result = pow(result, vec3(1.0 / gammaParam)); 

    FragColor = vec4(result, texColor.a);
}