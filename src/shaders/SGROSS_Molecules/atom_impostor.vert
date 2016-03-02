#version 450

uniform mat4 view;

in vec3  atom_position;
in float atom_radius;

out float vs_atom_radius;


void main()
{
    vs_atom_radius = atom_radius;

    gl_Position = view * vec4(atom_position, 1.0);
}
