#version 450

uniform mat4 view;

flat in vec3  gs_center_position_cam;

out vec4 frag_color;


void main()
{
    frag_color = vec4(normalize(abs((inverse(view) * vec4(gs_center_position_cam, 1.0)).xyz)), 1.0);
}
