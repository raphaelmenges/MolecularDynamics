//============================================================================
// Distributed under the MIT License. Author: Adrian Derstroff
//============================================================================

#version 430

// Color of impostor
out float vertRadius;
out vec4 pos;

// Uniforms
uniform float searchRadius;
uniform vec3 selectedAtomPosition;

// Main function
void main()
{
    /*
     * get atom center for every vertex id
     */
    gl_Position = vec4(selectedAtomPosition, 1);
    pos         = vec4(selectedAtomPosition, 1);
    vertRadius  = searchRadius;
}
