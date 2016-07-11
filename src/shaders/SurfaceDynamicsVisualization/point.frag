#version 430

flat in vec3 color;
flat in int index;
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 pickIndex;

void main()
{
    // Composite
    fragColor = color;

    // AtomIndex
    int r = (index & 0x000000FF) >>  0;
    int g = (index & 0x0000FF00) >>  8;
    int b = (index & 0x00FF0000) >> 16;
    pickIndex = vec3(
        float(r) / 255.0,
        float(g) / 255.0,
        float(b) / 255.0);
}
