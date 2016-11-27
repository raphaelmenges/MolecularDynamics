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

// Ascension
layout(std430, binding = 5) restrict readonly buffer AscensionBuffer
{
   float ascension[];
};

// Indices of atoms
layout(binding = 6, r32ui) readonly restrict uniform uimageBuffer Indices;

// Uniforms
uniform float probeRadius;
uniform int selectedIndex = 0;
uniform vec3 color;
uniform int frame;
uniform int atomCount;
uniform int smoothAnimationRadius;
uniform float smoothAnimationMaxDeviation;
uniform int frameCount;
uniform vec3 selectionColor;
uniform int localFrame;
uniform float ascensionChangeRadiusMultiplier;

// Global
int atomIndex;
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
    atomIndex = int(imageLoad(Indices, int(gl_VertexID)).x); // write it to global variable
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

    // Extract radius inclusive visualization of ascension angle
    float angle = ascension[(localFrame * atomCount) + int(atomIndex)];
    float originalRadius = radii[atomIndex] + probeRadius;
    vertRadius =
    originalRadius
        * (ascensionChangeRadiusMultiplier * abs(sin(angle))
            + (1.f - ascensionChangeRadiusMultiplier));

    // Set color
    if(atomIndex == selectedIndex)
    {
        vertColor = selectionColor;
    }
    else
    {
        vertColor = color;
    }

    // Set index
    vertIndex = atomIndex;
}
