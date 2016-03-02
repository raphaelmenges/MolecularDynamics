#version 450

uniform mat4 view;

in vec3   torus_center;
in float  torus_radius;
in vec3   tangent1_center;
in float  tangent1_radius;
in vec3   tangent2_center;
in float  tangent2_radius;

out float vs_torus_radius;
out vec4  vs_tangent1_center_cam;
out float vs_tangent1_radius;
out vec4  vs_tangent2_center_cam;
out float vs_tangent2_radius;


void main()
{
    vs_torus_radius        = torus_radius;
    vs_tangent1_center_cam = view * vec4(tangent1_center, 1.0);
    vs_tangent1_radius     = tangent1_radius;
    vs_tangent2_center_cam = view * vec4(tangent2_center, 1.0);
    vs_tangent2_radius     = tangent2_radius;

    gl_Position            = view * vec4(torus_center, 1.0);
}
