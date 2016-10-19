//============================================================================
// Distributed under the MIT License. Author: Adrian Derstroff
//============================================================================

#version 430

// Color of impostor
out float vertRadius;
out int   atomID;

// Struct for atom
struct AtomStruct
{
    vec3 center;
    float radius;
    vec4 proteinID;
};

// SSBOs
layout(std430, binding = 0) restrict readonly buffer AtomBuffer { AtomStruct atoms[];     };

// Main function
void main()
{
    /*
     * get atom center for every vertex id
     */
    int index = int(gl_VertexID);
    gl_Position = vec4(atoms[index].center, 1);
    atomID = index;

    /*
     * get the radius for the corresponding atom
     */
    vertRadius = atoms[index].radius;
}
