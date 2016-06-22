#version 430

// Attribute for posiiton
in vec3 position;

// Main function
void main()
{
    gl_Position = vec4(position, 1);
}
