#version 450

uniform mat4  projection;

flat in vec3  gs_atom_position_cam;
flat in float gs_atom_radius;

in vec3       gs_hit_position_cam;

out vec4      frag_color;
layout (depth_greater) out float gl_FragDepth;


void main()
{
    // line sphere intersection with speudo 3d impostor
    // https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection

    vec3 ray_direction = normalize(gs_hit_position_cam);

    float a = dot(ray_direction, -gs_atom_position_cam);
    float b = a * a - length(gs_atom_position_cam) * length(gs_atom_position_cam) + gs_atom_radius * gs_atom_radius;

    if (b < 0) discard; // no intercections

    float d = -a - sqrt(b); // just substract (+ lies always behind front point)
    vec3 real_hit_position_cam = d * ray_direction;

    vec3 normal = normalize(real_hit_position_cam - gs_atom_position_cam);
    frag_color = vec4(abs(normal), 1.0);
    //frag_color = vec4(normalize(abs(gs_atom_position_cam.xyz)), 1.0);

    vec4 real_position_screen = projection * vec4(real_hit_position_cam, 1.0);
    gl_FragDepth = 0.5 * (real_position_screen.z / real_position_screen.w) + 0.5;
}
