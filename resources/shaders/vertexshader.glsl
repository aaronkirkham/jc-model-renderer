#version 330 core

layout(location = 0) in vec4 vertex;
attribute vec4 colour;
uniform mat4 m_ModelViewProjection;
varying vec4 colour_v;

void main(void)
{
    gl_Position = m_ModelViewProjection * vertex;
    colour_v = colour;
}
