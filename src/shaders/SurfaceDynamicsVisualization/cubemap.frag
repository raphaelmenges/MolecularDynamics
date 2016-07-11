#version 430

// In / out
smooth in vec3 eyeDirection;
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 pickIndex;

// Uniforms
uniform samplerCube cubemap;

// Main function
void main()
{
    fragColor = texture(cubemap, eyeDirection).xyz;
    pickIndex = vec3(0,0,0);
}
