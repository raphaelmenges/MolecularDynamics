//============================================================================
// Distributed under the MIT License. Author: Adrian Derstroff
//============================================================================

#version 430

in vec2 uv;
flat in float radius;

out vec4 outColor;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    // Radius in UV space is 1 (therefore the scaling with 2 in geometry shader)

    /*
     * render only sphere on the triangle billboard
     */
    float distance = length(uv);
    if(distance > 1.0)
    {
        discard;
    }

    // Output color
    if(distance > 0.98) outColor = vec4(1, 1, 1, 1);
}
