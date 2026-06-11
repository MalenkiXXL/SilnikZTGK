#version 420 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D sceneColor;   
uniform sampler2D bloomBlur;    
// uniform sampler2D lutTexture; 

uniform float exposure;        
uniform float bloomStrength;   

void main()
{             
    vec3 hdrColor   = texture(sceneColor, TexCoords).rgb;      
    vec3 bloomColor = texture(bloomBlur,  TexCoords).rgb;
    
    hdrColor += bloomColor * bloomStrength; 
    
    vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
    
    const vec3 W = vec3(0.2125, 0.7154, 0.0721);
    vec3 intensity = vec3(dot(result, W));
    result = mix(intensity, result, 1.2); 
    
    result *= vec3(1.02, 1.00, 0.97); 

    FragColor = vec4(result, 1.0);
}
