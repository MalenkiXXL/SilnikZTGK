#version 330 core

in vec3 ourColor;
in vec2 TexCords;

out vec4 FragColor;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
	FragColor = mix(texture(texture1, TexCords), texture(texture2, TexCords), 0.2);
}