#version 420 core
layout (location = 0) out vec4 color;

in vec2 v_TexCoord;
in vec2 v_LocalPos;

uniform vec4 u_Color;
uniform sampler2D u_Texture;

// NOWE UNIFORMY: Do zaokr�glonych rog�w i pastelowej mi�kko�ci
uniform vec2 u_QuadSize;   // Rozmiar quada na ekranie (aby rogi nie by�y jajowate)
uniform float u_Radius;    // Promie� zaokr�glenia kraw�dzi w pikselach
uniform float u_Softness;  // Mi�kko�� (Antyaliasing) - im wy�sza, tym bardziej "rozmyty" brzeg

// Funkcja SDF (Signed Distance Field) dla zaokr�glonego prostok�ta
float roundedBoxSDF(vec2 CenterPosition, vec2 Size, float Radius) {
    return length(max(abs(CenterPosition) - Size + Radius, 0.0)) - Radius;
}

void main() {
    // Standardowe pobranie koloru z tekstury/u_Color
    vec4 texColor = texture(u_Texture, v_TexCoord) * u_Color;

    // 1. Przesuwamy uk�ad wsp�rz�dnych na �rodek Quada (od -0.5 do 0.5) i skalujemy
    vec2 centerPos = (v_LocalPos - 0.5) * u_QuadSize;
    vec2 halfSize = u_QuadSize * 0.5;

    // 2. Liczymy dystans od idealnie zaokr�glonej kraw�dzi
    // Wymuszamy, by promie� nie by� wi�kszy ni� po�owa najkr�tszego boku
    float actualRadius = min(u_Radius, min(halfSize.x, halfSize.y));
    float distance = roundedBoxSDF(centerPos, halfSize, actualRadius);

    // 3. Wyg�adzamy kraw�d� ucinaj�c piksele poza dystansem 0.0
    // Softness = 1.0-1.5 daje idealny, g�adki antyaliasing. Wy�sze warto�ci zrobi� chmurk�!
    float smoothedAlpha = 1.0 - smoothstep(0.0, u_Softness, distance);

    // 4. Aplikujemy wyg�adzon� Alf� do naszego koloru
    color = vec4(texColor.rgb, texColor.a * smoothedAlpha);
}