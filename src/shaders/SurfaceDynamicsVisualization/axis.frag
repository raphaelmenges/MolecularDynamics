#version 430

uniform vec3 color;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 pickIndex;

void main()
{
    fragColor = color;
    pickIndex = vec3(0,0,0);
}
