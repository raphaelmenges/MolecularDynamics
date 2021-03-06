//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#version 430

// Attribute for posiiton
in vec3 position;
out vec3 vertColor;

// ## Trajectory SSBO
struct Position
{
    float x,y,z;
};

layout(std430, binding = 1) restrict readonly buffer TrajectoryBuffer
{
   Position trajectory[];
};

// ## Relative positions of samples SSBO
layout(std430, binding = 2) restrict readonly buffer RelativePositionBuffer
{
   Position relativePosition[];
};

// ## Image buffer with classification
layout(binding = 3, r32ui) restrict readonly uniform uimageBuffer Classification;

// ## Uniforms
uniform int sampleCount;
uniform int frame;
uniform int atomCount;
uniform int integerCountPerSample;
uniform int localFrame;
uniform vec3 internalColor;
uniform vec3 surfaceColor;

// Main function
void main()
{
    // Extract index of atom
    int atomIndex = int(gl_VertexID) / sampleCount;

    // Extract index of sample
    int sampleIndex = int(gl_VertexID) - (atomIndex * sampleCount);

    // Calculate position
    Position atomPosition = trajectory[(frame*atomCount) + atomIndex];
    Position relativeSamplePosition = relativePosition[(atomIndex * sampleCount) + sampleIndex];
    gl_Position = vec4(
        atomPosition.x + relativeSamplePosition.x,
        atomPosition.y + relativeSamplePosition.y,
        atomPosition.z + relativeSamplePosition.z,
        1);

    // Calculate indices to look up classification
    int uintIndex =
        (atomIndex * sampleCount * integerCountPerSample) // offset for current atom's samples
        + (sampleIndex *  integerCountPerSample) // offset for current sample's slot
        + (localFrame / 32); // offset for unsigned int which has to be read
    int bitIndex = localFrame - (32 * int(localFrame / 32)); // bit index within unsigned integer

    // Fetch classification
    if(((uint(imageLoad(Classification, uintIndex).x) >> uint(bitIndex)) & 1) > 0)
    {
        vertColor = surfaceColor;
    }
    else
    {
        vertColor = internalColor;
    }
}
