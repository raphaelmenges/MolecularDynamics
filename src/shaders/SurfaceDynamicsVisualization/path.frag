#version 430

// In / out
layout(location = 0) out vec4 fragColor;
in flat vec3 color;

// Main function
void main()
{
    fragColor = vec4(color, 1);
}
