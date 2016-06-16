#version 430

// Color of rendered point
out vec3 vertColor;

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
layout(binding = 1, r32ui) readonly restrict uniform uimageBuffer Indices;

// Uniforms
uniform int selectedIndex;
uniform vec3 color;

// Main function
void main()
{
    // Extract position
    int index = int(imageLoad(Indices, int(gl_VertexID)).x);
    gl_Position = vec4(atoms[index].center, 1);

    // Set color
    if(index == selectedIndex)
    {
        vertColor = vec3(0,1,0);
    }
    else
    {
        vertColor = color;
    }
}
