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
uniform int atomIndex;

// Main function
void main()
{
    // Extract position
    vec3 position = atoms[atomIndex].center;
    vec4 viewPosition = view * vec4(position, 1);
    viewPosition.z += 0.015; // move towards camera since all protein's atoms get rendered before that and z buffer is filled
    gl_Position = projection * viewPosition;

    // Set color
    col = vec3(0,1,0);
}
