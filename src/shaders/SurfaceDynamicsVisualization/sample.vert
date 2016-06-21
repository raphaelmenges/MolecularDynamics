#version 430

// Attribute for posiiton
in vec3 position;

// Uniforms
uniform mat4 projection;
uniform mat4 view;

// Main function
void main()
{
    gl_Position = projection * view * vec4(position, 1);
}
