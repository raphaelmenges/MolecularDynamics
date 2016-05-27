#version 430

// Color of rendered point
out vec3 col;

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
uniform mat4 projection;
uniform mat4 view;
uniform int selectedIndex;
uniform vec3 color;

// Main function
void main()
{
    // Extract position
    int index = int(imageLoad(Indices,int(gl_VertexID)).x);
    vec3 position = atoms[index].center;
    vec4 viewPosition = view * vec4(position, 1);
    viewPosition.z += 0.01; // move towards camera since all protein's atoms get rendered before that and z buffer is filled
    gl_Position = projection * viewPosition;

    // Set color
    if(index == selectedIndex)
    {
        col = vec3(0,1,0);
    }
    else
    {
        col = color;
    }
}
