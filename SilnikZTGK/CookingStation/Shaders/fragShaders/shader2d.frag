#version 420 core
in vec4 v_Color;
in vec2 v_TexCoord;
in float v_TexIndex;
in vec2 v_QuadSize;
in float v_Radius;
out vec4 FragColor;

uniform sampler2D u_Textures[16];

void main() {
    int index = int(v_TexIndex);
    vec4 sampled = vec4(1.0);

    switch(index) {
        case  0: sampled = texture(u_Textures[0],  v_TexCoord); break;
        case  1: sampled = texture(u_Textures[1],  v_TexCoord); break;
        case  2: sampled = texture(u_Textures[2],  v_TexCoord); break;
        case  3: sampled = texture(u_Textures[3],  v_TexCoord); break;
        case  4: sampled = texture(u_Textures[4],  v_TexCoord); break;
        case  5: sampled = texture(u_Textures[5],  v_TexCoord); break;
        case  6: sampled = texture(u_Textures[6],  v_TexCoord); break;
        case  7: sampled = texture(u_Textures[7],  v_TexCoord); break;
        case  8: sampled = texture(u_Textures[8],  v_TexCoord); break;
        case  9: sampled = texture(u_Textures[9],  v_TexCoord); break;
        case 10: sampled = texture(u_Textures[10], v_TexCoord); break;
        case 11: sampled = texture(u_Textures[11], v_TexCoord); break;
        case 12: sampled = texture(u_Textures[12], v_TexCoord); break;
        case 13: sampled = texture(u_Textures[13], v_TexCoord); break;
        case 14: sampled = texture(u_Textures[14], v_TexCoord); break;
        case 15: sampled = texture(u_Textures[15], v_TexCoord); break;
    }

    if (v_Radius < -0.5) {
        float distance = sampled.a;
      
        float smoothing = fwidth(distance); 
        float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
        
        FragColor = vec4(v_Color.rgb, v_Color.a * alpha);
        
        if (FragColor.a < 0.01) discard;
        return; 
    }

    vec4 texColor = v_Color * sampled;

    if (v_Radius > 0.0 && v_QuadSize.x > 0.0 && v_QuadSize.y > 0.0) {
        vec2 pixelPos = v_TexCoord * v_QuadSize;
        vec2 halfSize = v_QuadSize * 0.5;
        vec2 centerOffset = abs(pixelPos - halfSize);
        vec2 q = centerOffset - (halfSize - vec2(v_Radius));
        float dist = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - v_Radius;
        
        float alpha = 1.0 - smoothstep(-0.5, 1.0, dist);
        texColor.a *= alpha;
    }

    if (texColor.a < 0.01) discard;
    FragColor = texColor;
}