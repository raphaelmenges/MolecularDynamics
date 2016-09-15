//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#version 430

// Color and radius of impostor
out vec3 vertColor;
out float vertRadius;

// Index of atom
out int vertIndex;

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

// Uniforms
uniform float probeRadius;
uniform int selectedIndex = 0;
uniform int frame;
uniform int atomCount;
uniform int smoothAnimationRadius;
uniform float smoothAnimationMaxDeviation;
uniform int frameCount;
uniform vec3 color;
uniform int atomIndex;

// Global
vec3 centerAtFrame;

// Accumulate center at frame
void accumulateCenter(
    int accFrame,
    inout vec3 accCenters,
    inout int accCount)
{
    // Extract center at that frame
    Position position = trajectory[(accFrame*atomCount) + atomIndex];
    vec3 center = vec3(position.x, position.y, position.z);

    // Check whether center is not too far away
    float distanceToFramesCenter = distance(centerAtFrame, center);
    if(distanceToFramesCenter < smoothAnimationMaxDeviation)
    {
        accCenters += center;
        accCount++;
    }
}

// Main function
void main()
{
    // Extract center at frame which is given
    Position position = trajectory[(frame*atomCount) + atomIndex];
    centerAtFrame = vec3(position.x, position.y, position.z); // write it to global variable

    // Calculate loop bounds for smoothing
    int lowerBound = max(0, frame - smoothAnimationRadius);
    int upperBound = min(frameCount - 1, frame + smoothAnimationRadius);
    int accCount = 0;
    vec3 accCenters = vec3(0,0,0);

    // Accumulate centers in frames below current one
    for(int i = lowerBound; i < frame; i++)
    {
        accumulateCenter(i, accCenters, accCount);
    }

    // Accumulate centers in frames above current one
    for(int i = frame + 1; i <= upperBound; i++)
    {
        accumulateCenter(i, accCenters, accCount);
    }

    // Extract center from accumulated ones
    vec3 center = (accCenters + centerAtFrame) / (accCount + 1);
    gl_Position = vec4(center, 1);

    // Extract radius
    vertRadius = radii[atomIndex] + probeRadius;

    // Set color
    vertColor = color;

    // Set index (not used for pick index since framebuffer has no attachment for it)
    vertIndex = atomIndex;
}
