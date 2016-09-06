#version 430

// Color of impostor
out vec3 vertColor;
out float vertRadius;
out int isSelected;

// Struct for atom
struct AtomStruct
{
    vec3 center;
    float radius;
    vec4 proteinID;
};

// SSBOs
layout(std430, binding = 0) restrict readonly buffer AtomBuffer { AtomStruct atoms[];     };
layout(std430, binding = 1) buffer SearchResultBuffer           { int        searchRes[]; };

// Uniforms
uniform vec3 cameraWorldPos;
uniform float probeRadius;
uniform int selectedIndex;
uniform int proteinNum;
uniform int selectedProtein;

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
     * check if atom is of selected protein
     */
    isSelected = abs(int(atoms[index].proteinID.x) - selectedProtein);

    /*
     * color selected atom different to all atoms
     */
    if (index == selectedIndex)
    {
        vertColor = vec3(1,0,0);
    }
    else if (searchRes[index] == 1)
    {
        vertColor = vec3(0,1,0);
    }
    else if (searchRes[index] == 2)
    {
        vertColor = vec3(0,0,1);
    }
    else if (searchRes[index] == 3)
    {
        vertColor = vec3(0.6,0.6,0.6);
    }
    else if (searchRes[index] == 4)
    {
        vertColor = vec3(1,1,0);
    }
    else
    {
        float proteinIdx = atoms[index].proteinID.x;
        float PI = 3.1415926;
        float colorF = PI*(float(proteinIdx)/proteinNum);
        float sinC = sin(colorF);
        float cosC = cos(colorF);
        vec3 proteinColor = vec3(sinC, cosC, min(1,sinC+cosC));
        vertColor = proteinColor;
    }
}
