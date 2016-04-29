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

// Uniforms
uniform mat4 projection;
uniform mat4 view;

// Main function
void main()
{
    // Extract position
    vec3 position = atoms[int(gl_VertexID)].center;
    gl_Position = projection * view * vec4(position, 1);

    // Set color
    col = vec3(1,1,1);
}
