#version 430

// In / out
smooth in vec3 eyeDirection;
layout(location = 0) out vec4 fragColor;

// Uniforms
uniform samplerCube cubemap;

// Main function
void main()
{
    fragColor = texture(cubemap, eyeDirection);
}
