#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 3) out;

uniform mat4 projection;

in vec4       vs_atom1_position_cam[];
in vec4       vs_atom2_position_cam[];
in vec4       vs_atom3_position_cam[];

flat out vec3 gs_center_position_cam;


void draw_vertex(vec4 position)
{
    gl_Position = projection * position;

    EmitVertex();
}

void main()
{
    gs_center_position_cam = ((vs_atom1_position_cam[0] + vs_atom2_position_cam[0] + vs_atom3_position_cam[0]) / 3.0).xyz;

    draw_vertex(vs_atom1_position_cam[0]);
    draw_vertex(vs_atom2_position_cam[0]);
    draw_vertex(vs_atom3_position_cam[0]);

    EndPrimitive();
}
