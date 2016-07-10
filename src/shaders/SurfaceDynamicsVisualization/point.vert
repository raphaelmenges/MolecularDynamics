#version 430

// Color of rendered point
out vec3 vertColor;

// Trajectory
struct Position
{
    float x,y,z;
};

layout(std430, binding = 1) restrict readonly buffer TrajectoryBuffer
{
   Position trajectory[];
};

// Indices of surface atoms
layout(binding = 2, r32ui) readonly restrict uniform uimageBuffer Indices;

// Uniforms
uniform int selectedIndex;
uniform vec3 color;
uniform int frame;
uniform int atomCount;

// Main function
void main()
{
    // Extract center
    int atomIndex = int(imageLoad(Indices, int(gl_VertexID)).x);
    Position position = trajectory[(frame*atomCount) + atomIndex];
    gl_Position = vec4(position.x, position.y, position.z, 1);

    // Set color
    if(atomIndex == selectedIndex)
    {
        vertColor = vec3(0,1,0);
    }
    else
    {
        vertColor = color;
    }
}
