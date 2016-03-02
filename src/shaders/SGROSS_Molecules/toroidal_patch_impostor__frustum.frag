#version 450

flat in vec3  gs_torus_center_cam;
in vec3       gs_hit_position_cam;

out vec4      frag_color;


void main()
{
    frag_color = vec4(normalize(abs(gs_hit_position_cam - gs_torus_center_cam)), 1.0);
}
