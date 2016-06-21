#version 430

// Color of impostor
out vec3 vertColor;
out float vertRadius;

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
uniform vec3 cameraWorldPos;
uniform float probeRadius;
uniform int selectedIndex;
uniform vec3 color;

// Main function
void main()
{
    /*
     * get atom center for every vertex id
     */
    int index = int(gl_VertexID);
    gl_Position = vec4(atoms[index].center, 1);

    /*
     * get the radius for the corresponding atom
     */
    vertRadius = atoms[index].radius + probeRadius;

    /*
     * color selected atom different to all atoms
     */
    if(index == selectedIndex)
    {
        vertColor = vec3(0,1,0);
    }
    else
    {
        vertColor = color;
    }
}
