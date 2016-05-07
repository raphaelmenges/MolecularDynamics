#version 430

// Color of impostor
out vec3 vertColor;
out float vertRadius;

// Struct for atom
struct AtomStruct
{
    vec3 center;
    float radius;
};

// SSBOs
layout(std430, binding = 0) restrict readonly buffer AtomBuffer
{
   AtomStruct atoms[];
};

// Uniforms
uniform vec3 cameraWorldPos;
uniform float probeRadius;
uniform int atomIndex;

// Main function
void main()
{
    // Extract position
    vec3 center = atoms[atomIndex].center;
    vec3 vector = normalize(cameraWorldPos - center);
    gl_Position = vec4(center + 0.015 * vector, 1);

    // Extract radius
    vertRadius = atoms[atomIndex].radius + probeRadius;

    // Set color
    vertColor = vec3(0,1,0);
}
