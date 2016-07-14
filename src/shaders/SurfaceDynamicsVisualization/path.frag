#version 430

// In / out
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 pickIndex;
in flat vec3 color;

// Main function
void main()
{
    fragColor = color;
    pickIndex = vec3(0,0,0);
}
