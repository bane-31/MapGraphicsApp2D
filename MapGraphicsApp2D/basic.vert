#version 330 core

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec4 inCol;
layout(location = 2) in vec2 inTex;
out vec2 chTex;
out vec4 chCol;

void main()
{
    gl_Position = vec4(inPos.x, inPos.y, 0.0, 1.0);
    chTex = inTex;
    chCol = inCol;
}