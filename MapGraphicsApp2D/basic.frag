#version 330 core

in vec4 chCol;
in vec2 chTex;
out vec4 outCol;

uniform sampler2D uTex;
uniform bool uUseTex;
uniform bool uUseCol;

void main()
{
	outCol = vec4(1.0);
	if (uUseTex) outCol *= texture(uTex, chTex);
	if (uUseCol) outCol *= chCol;
}
