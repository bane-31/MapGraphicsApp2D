#version 330 core

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec4 inCol;
layout(location = 2) in vec2 inTex;
out vec2 chTex;
out vec4 chCol;

uniform vec2 uCenter;
uniform vec2 uRange;
uniform vec2 uPos;
uniform vec2 uScale;

void main()
{
    gl_Position = vec4(inPos * uScale + uPos, 0.0, 1.0);
    chTex = uCenter + (inTex - vec2(0.5)) * uRange;
    chCol = inCol;
}