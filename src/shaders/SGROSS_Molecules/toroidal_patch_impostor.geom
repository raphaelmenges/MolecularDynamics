#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 14) out;

uniform mat4   projection;

in float       vs_torus_radius[];
in vec4        vs_tangent1_center_cam[];
in float       vs_tangent1_radius[];
in vec4        vs_tangent2_center_cam[];
in float       vs_tangent2_radius[];

flat out vec3  gs_torus_center_cam;
flat out float gs_torus_radius;
flat out vec3  gs_tangent1_center_cam;
flat out vec3  gs_tangent2_center_cam;
out vec3       gs_hit_position_cam;


void draw_vertex(vec4 position)
{
    gs_hit_position_cam = position.xyz;
    gl_Position = projection * position;

    EmitVertex();
}

void main()
{
    gs_torus_center_cam    = gl_in[0].gl_Position.xyz;
    gs_torus_radius        = vs_torus_radius[0];
    gs_tangent1_center_cam = vs_tangent1_center_cam[0].xyz;
    gs_tangent2_center_cam = vs_tangent2_center_cam[0].xyz;

    vec4 center_axis = vs_tangent2_center_cam[0] - vs_tangent1_center_cam[0];
    vec4 u;

    if (center_axis.z != 0 && center_axis.y != 0)
    {
        u  = normalize(vec4(0.0, -center_axis.z, center_axis.y, 0.0));
    }
    else
    {
        u = normalize(vec4(center_axis.y, -center_axis.x, 0.0, 0.0));
    }

    vec4 v = normalize(vec4(cross(u.xyz, center_axis.xyz), 0.0));


    vec4 vertex_1 = vs_tangent1_center_cam[0] - vs_tangent1_radius[0] * u + vs_tangent1_radius[0] * v;
    vec4 vertex_2 = vs_tangent1_center_cam[0] + vs_tangent1_radius[0] * u + vs_tangent1_radius[0] * v;
    vec4 vertex_3 = vs_tangent1_center_cam[0] - vs_tangent1_radius[0] * u - vs_tangent1_radius[0] * v;
    vec4 vertex_4 = vs_tangent1_center_cam[0] + vs_tangent1_radius[0] * u - vs_tangent1_radius[0] * v;

    vec4 vertex_5 = vs_tangent2_center_cam[0] + vs_tangent2_radius[0] * u + vs_tangent2_radius[0] * v;
    vec4 vertex_6 = vs_tangent2_center_cam[0] - vs_tangent2_radius[0] * u + vs_tangent2_radius[0] * v;
    vec4 vertex_7 = vs_tangent2_center_cam[0] - vs_tangent2_radius[0] * u - vs_tangent2_radius[0] * v;
    vec4 vertex_8 = vs_tangent2_center_cam[0] + vs_tangent2_radius[0] * u - vs_tangent2_radius[0] * v;

    draw_vertex(vertex_8);
    draw_vertex(vertex_4);
    draw_vertex(vertex_7);
    draw_vertex(vertex_3);
    draw_vertex(vertex_1);
    draw_vertex(vertex_4);
    draw_vertex(vertex_2);
    draw_vertex(vertex_8);
    draw_vertex(vertex_5);
    draw_vertex(vertex_7);
    draw_vertex(vertex_6);
    draw_vertex(vertex_1);
    draw_vertex(vertex_5);
    draw_vertex(vertex_2);

    EndPrimitive();
}
