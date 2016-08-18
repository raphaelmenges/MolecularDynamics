//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#version 430

// Attribute for posiiton
in vec3 position;

// Uniforms
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

// Main function
void main()
{
    gl_Position = projection * view * model * vec4(position, 1);
}
