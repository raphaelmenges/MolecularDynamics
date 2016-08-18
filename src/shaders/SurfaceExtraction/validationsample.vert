//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#version 430

// Attribute for posiiton
in vec3 position;
out vec3 vertColor;

// Uniforms
uniform vec3 color;

// Main function
void main()
{
    gl_Position = vec4(position, 1);
    vertColor = color;
}
