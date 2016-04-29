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
uniform float probeRadius;

// Main function
void main()
{
    // Extract position
    int index = int(gl_VertexID);
    gl_Position = vec4(atoms[index].center, 1);

    // Extract radius
    vertRadius = atoms[index].radius + probeRadius;

    // Set color
    vertColor = vec3(0.5,0.5,0.5);
}
