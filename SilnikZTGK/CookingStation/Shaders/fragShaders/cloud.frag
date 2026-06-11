#version 420 core
out vec4 FragColor;

in float v_uvOffset;
in vec2 TexCoords;
in vec2 TexCoords2;
in vec3 Normal;
in vec3 FragPos;
in vec4 FragPosLightSpace;

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
uniform sampler2D shadowMap;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.z > 1.0) return 0.0;

    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    return shadow;
}

void main()
{
    vec3 baseColor = texture(texture_diffuse1, TexCoords).rgb;

    if(useTexture2) {
        vec4 tex2Sample = texture(texture_diffuse2, TexCoords2);
        float lum = max(tex2Sample.r, max(tex2Sample.g, tex2Sample.b));
        vec3 trueColor = tex2Sample.rgb / max(lum, 0.001);
        baseColor = mix(baseColor, trueColor, lum * tex2Sample.a);
    }

    float ambientStrength = 0.6;
    vec3 ambient = ambientStrength * vec3(1.0);

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-u_SunDir);


    float diff = dot(norm, lightDir) * 0.5 + 0.5;
    diff = diff * diff;

    vec3 diffuse = (diff * 0.40) * u_LightColor;

    float shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);

    float cloudShadowStrength = 0.3;
    vec3 result = (ambient + (1.0 - (shadow * cloudShadowStrength)) * diffuse) * baseColor;

    float gammaParam = 1.4;
    result = pow(result, vec3(1.0 / gammaParam));

    FragColor = vec4(result, 1.0);
}