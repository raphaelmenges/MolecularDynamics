#version 430

// Color of rendered point
out vec3 col;

// Struct for atom
struct AtomStruct
{
    vec3 position;
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
uniform mat4 projection;
uniform mat4 view;

// Main function
void main()
{
    // Extract position
    int index = int(imageLoad(surfaceAtomImage,int(gl_VertexID)).x);
    vec3 position = atoms[index].position;
    gl_Position = projection * view * vec4(0,0,0, 1);

    // Set color
    col = vec3(1,0,0);
}
