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

// Main function
void main()
{
    // Extract position
    int index = int(imageLoad(surfaceAtomImage,int(gl_VertexID)).x);
    gl_Position = vec4(atoms[index].center, 1);

    // Extract radius
    vertRadius =  atoms[index].radius;

    // Set color
    vertColor = vec3(0.5,0.5,0.5);
}
