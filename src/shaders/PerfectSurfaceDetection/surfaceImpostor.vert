#version 450

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

// Indices of surface atoms
layout(binding = 1, r32ui) readonly restrict uniform uimageBuffer surfaceAtomImage;

// Uniforms
uniform vec3 cameraWorldPos;
uniform float probeRadius;

// Main function
void main()
{
    // Extract position
    int index = int(imageLoad(surfaceAtomImage,int(gl_VertexID)).x);
    vec3 center = atoms[index].center;
    vec3 vector = normalize(cameraWorldPos - center);
    gl_Position = vec4(center + 0.01 * vector, 1);

    // Extract radius
    vertRadius =  atoms[index].radius + probeRadius;

    // Set color
    vertColor = vec3(1,0,0);
}
