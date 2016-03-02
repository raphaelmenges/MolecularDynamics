#version 450

uniform mat4 view;

in vec3   probe_position;
in vec3   atom1_position;
in vec3   atom2_position;
in vec3   atom3_position;

out vec4  vs_atom1_position_cam;
out vec4  vs_atom2_position_cam;
out vec4  vs_atom3_position_cam;


void main()
{
    vs_atom1_position_cam = view * vec4(atom1_position, 1.0);
    vs_atom2_position_cam = view * vec4(atom2_position, 1.0);
    vs_atom3_position_cam = view * vec4(atom3_position, 1.0);

    gl_Position           = view * vec4(probe_position, 1.0);
}
