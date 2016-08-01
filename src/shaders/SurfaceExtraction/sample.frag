#version 430

flat in vec3 col;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(col, 1.0);
}
