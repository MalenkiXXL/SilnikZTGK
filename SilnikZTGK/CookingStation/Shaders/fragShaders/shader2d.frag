#version 420 core
in vec4 v_Color;
in vec2 v_TexCoord;
in float v_TexIndex;
in vec2 v_QuadSize;
in float v_Radius;
out vec4 FragColor;

uniform sampler2D u_Textures[16];

void main() {
    vec4 texColor = v_Color;
    int index = int(v_TexIndex);

    switch(index) {
        case  0: texColor *= texture(u_Textures[0],  v_TexCoord); break;
        case  1: texColor *= texture(u_Textures[1],  v_TexCoord); break;
        case  2: texColor *= texture(u_Textures[2],  v_TexCoord); break;
        case  3: texColor *= texture(u_Textures[3],  v_TexCoord); break;
        case  4: texColor *= texture(u_Textures[4],  v_TexCoord); break;
        case  5: texColor *= texture(u_Textures[5],  v_TexCoord); break;
        case  6: texColor *= texture(u_Textures[6],  v_TexCoord); break;
        case  7: texColor *= texture(u_Textures[7],  v_TexCoord); break;
        case  8: texColor *= texture(u_Textures[8],  v_TexCoord); break;
        case  9: texColor *= texture(u_Textures[9],  v_TexCoord); break;
        case 10: texColor *= texture(u_Textures[10], v_TexCoord); break;
        case 11: texColor *= texture(u_Textures[11], v_TexCoord); break;
        case 12: texColor *= texture(u_Textures[12], v_TexCoord); break;
        case 13: texColor *= texture(u_Textures[13], v_TexCoord); break;
        case 14: texColor *= texture(u_Textures[14], v_TexCoord); break;
        case 15: texColor *= texture(u_Textures[15], v_TexCoord); break;
    }

    // Zaokraglanie rogow (SDF rounded box)
    if (v_Radius > 0.0 && v_QuadSize.x > 0.0 && v_QuadSize.y > 0.0) {
        // Konwertuj UV [0,1] na wspolrzedne pikselowe
        vec2 pixelPos = v_TexCoord * v_QuadSize;

        // Polowa rozmiaru quada
        vec2 halfSize = v_QuadSize * 0.5;

        // Dystans od centrum (wartosci bezwzgledne - symetria)
        vec2 centerOffset = abs(pixelPos - halfSize);

        // SDF zaokraglonego prostokata
        vec2 q = centerOffset - (halfSize - vec2(v_Radius));
        float dist = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - v_Radius;

        // dist < 0 = wewnatrz, dist > 0 = na zewnatrz
        float alpha = 1.0 - smoothstep(-0.5, 1.0, dist);
        texColor.a *= alpha;
    }

    if (texColor.a < 0.01) discard;
    FragColor = texColor;
}
