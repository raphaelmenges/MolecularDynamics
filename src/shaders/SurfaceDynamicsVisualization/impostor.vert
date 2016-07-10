#version 430

// Color of impostor
out vec3 vertColor;
out float vertRadius;

// Radii
layout(std430, binding = 0) restrict readonly buffer RadiiBuffer
{
   float radii[];
};

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
uniform vec3 cameraWorldPos;
uniform float probeRadius;
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

    // Extract radius
    vertRadius = radii[atomIndex] + probeRadius;

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
