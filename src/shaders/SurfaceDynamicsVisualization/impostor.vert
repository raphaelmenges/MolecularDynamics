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
layout(std430, binding = 1) restrict readonly buffer TrajectoryBuffer
{
   vec3 trajectory[];
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
    int index = int(imageLoad(Indices, int(gl_VertexID)).x);
    gl_Position = vec4(trajectory[(frame*atomCount) + index], 1);

    // Extract radius
    vertRadius = radii[index] + probeRadius;

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
