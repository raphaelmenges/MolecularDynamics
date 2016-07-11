#version 430

flat in vec3 color;
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 atomId;

void main()
{
    fragColor = color;
    atomId = vec3(0,1,0);
}
