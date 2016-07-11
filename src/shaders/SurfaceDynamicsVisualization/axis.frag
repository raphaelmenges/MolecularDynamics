#version 430

uniform vec3 color;

layout(location = 0) out vec3 fragColor;

void main()
{
    fragColor = color;
}
