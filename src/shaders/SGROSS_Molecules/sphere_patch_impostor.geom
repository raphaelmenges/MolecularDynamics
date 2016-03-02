#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 5) out;

uniform mat4  projection;
uniform float probe_radius;

in vec4       vs_atom1_position_cam[];
in vec4       vs_atom2_position_cam[];
in vec4       vs_atom3_position_cam[];

flat out vec3 gs_probe_position_cam;
flat out vec3 gs_atom1_position_cam;
flat out vec3 gs_atom2_position_cam;
flat out vec3 gs_atom3_position_cam;
out vec3      gs_hit_position_cam;


void draw_vertex(vec3 position)
{
    gs_hit_position_cam = position;
    gl_Position         = projection * vec4(position, 1.0);

    EmitVertex();
}

void main()
{
    gs_probe_position_cam = gl_in[0].gl_Position.xyz;
    gs_atom1_position_cam = vs_atom1_position_cam[0].xyz;
    gs_atom2_position_cam = vs_atom2_position_cam[0].xyz;
    gs_atom3_position_cam = vs_atom3_position_cam[0].xyz;

    draw_vertex(gs_atom1_position_cam);
    draw_vertex(gs_atom2_position_cam);
    draw_vertex(gs_probe_position_cam);
    draw_vertex(gs_atom3_position_cam);
    draw_vertex(gs_atom1_position_cam);

    EndPrimitive();
}
