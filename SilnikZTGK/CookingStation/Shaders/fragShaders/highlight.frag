#version 330 core
out vec4 FragColor;

uniform vec3 u_HighlightColor; // Przeka¿emy tu np. jasny b³êkit lub ¿ó³æ z kodu C++
uniform float u_Time;          // Ca³kowity czas gry (do animacji pulsowania)

void main()
{
    // Animacja pulsowania bazuj¹ca na funkcji sinus (wartoœci przenikaj¹ miêdzy 0.3 a 0.7)
    float alpha = 0.5 + 0.2 * sin(u_Time * 6.0); 
    FragColor = vec4(u_HighlightColor, alpha);
}