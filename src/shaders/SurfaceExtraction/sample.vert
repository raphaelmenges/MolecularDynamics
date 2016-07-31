#version 430

// Attribute for posiiton
in vec3 position;
out vec3 vertColor;

// ## SSBOs

// Trajectory
struct Position
{
    float x,y,z;
};

layout(std430, binding = 1) restrict readonly buffer TrajectoryBuffer
{
   Position trajectory[];
};

layout(std430, binding = 2) restrict readonly buffer RelativePositionBuffer
{
   Position relativePosition[];
};

layout(std430, binding = 3) restrict readonly buffer SurfaceClassificationBuffer
{
   unsigned int classification[];
};

// ## Uniforms
uniform int sampleCount;
uniform int frame;
uniform int atomCount;

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

    // Color
    vertColor = vec3(0,0,1);
}
