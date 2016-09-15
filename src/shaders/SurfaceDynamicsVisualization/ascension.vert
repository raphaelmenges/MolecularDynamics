//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#version 430

// Definitions
const float TWO_PI = 2.0 * 3.14159265359;

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

// Ascension (angle for determining hue)
layout(std430, binding = 2) restrict readonly buffer AscensionBuffer
{
   float ascension[];
};

// Uniforms
uniform float probeRadius;
uniform int selectedIndex = 0;
uniform int frame;
uniform int atomCount;
uniform int smoothAnimationRadius;
uniform float smoothAnimationMaxDeviation;
uniform int frameCount;
uniform int ascensionFrame;
uniform float ascensionColorOffsetAngle;
uniform vec3 selectionColor;
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
    // Extract center at frame which is given. Unlike hull shader, here are atom indices directly given by vertex id
    atomIndex = int(gl_VertexID); // write it to global variable
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

    // Extract ascension angle
    float angle = ascension[(ascensionFrame * atomCount) + int(atomIndex)];

    // Extract radius
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
        // Determine angle in hue by adding the given offset
        float hueAngle = angle + ascensionColorOffsetAngle;
        float h = mod(hueAngle, TWO_PI);
        float s = 1.0;
        float v = 1.0;

        // Get color from angle
        float hDegree = (h / TWO_PI) * 360.0;
        float c = v * s;
        float x = c * (1-abs(mod(hDegree / 60.0, 2.0) - 1.0));
        float m = v - c;

        // Set color
        if(0.0 <= hDegree && hDegree < 60.0)
        {
            vertColor = vec3(c, x, 0);
        }
        else if(60.0 <= hDegree && hDegree < 120.0)
        {
            vertColor = vec3(x, c, 0);
        }
        else if(120.0 <= hDegree && hDegree < 180.0)
        {
            vertColor = vec3(0, c, x);
        }
        else if(180.0 <= hDegree && hDegree < 240.0)
        {
            vertColor = vec3(0, x, c);
        }
        else if(240.0 <= hDegree && hDegree < 300.0)
        {
            vertColor = vec3(x, 0, c);
        }
        else if(300.0 <= hDegree && hDegree < 360.0)
        {
            vertColor = vec3(c, 0, x);
        }
        vertColor += m;
    }

    // Set index
    vertIndex = atomIndex + 1; // plus one to distinguish from nothing!
}
