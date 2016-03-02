#version 450

uniform mat4  projection;

flat in vec3  gs_atom_position_cam;
flat in float gs_atom_radius;

in vec3       gs_hit_position_cam;

out vec4      frag_color;
layout (depth_greater) out float gl_FragDepth;

const int     raymarch_steps_max = 128;
const float   raymarch_epsilon   = 0.001;


float distance_to_surface(vec3 position)
{
    return length(position - gs_atom_position_cam) - gs_atom_radius;
}

void main()
{
    vec3 ray_direction = normalize(gs_hit_position_cam);

    float distance = distance_to_surface(gs_hit_position_cam);

    vec3 real_hit_position_cam = gs_hit_position_cam;


    for (int step = 0; step < raymarch_steps_max; step++)
    {
        real_hit_position_cam += ray_direction * distance;

        distance = distance_to_surface(real_hit_position_cam);

        if (abs(distance) < raymarch_epsilon)
        {
            break;
        }

        // temp test (should test for outside cube)
        // but raymarching isnt used so why invest more time in this :>
        if (step + 1 >= raymarch_steps_max) discard;
    }

    vec3 normal = normalize(real_hit_position_cam - gs_atom_position_cam);
    frag_color = vec4(abs(normal), 1.0);

    vec4 real_position_screen = projection * vec4(real_hit_position_cam, 1.0);
    gl_FragDepth = 0.5 * (real_position_screen.z / real_position_screen.w) + 0.5;


    // --- QUAD

    /*
    float s = dot(gs_atom_position_cam.xyz, -gs_atom_position_cam.xyz) / dot(-gs_atom_position_cam.xyz, ray_direction);

    vec3 hit_position_on_plane = s * ray_direction;
    float distance_on_plane    = length(hit_position_on_plane - gs_atom_position_cam.xyz);

    if (distance_on_plane > gs_atom_radius)
    {
        discard;
    }

    vec4 pos     = gs_atom_position_cam;
    pos.xyz     += sqrt(gs_atom_radius * gs_atom_radius - distance_on_plane * distance_on_plane) * -ray_direction;
    pos          = projection * pos;
    gl_FragDepth = 0.5 * (pos.z / pos.w) + 0.5;

    frag_color = vec4(normalize(abs((inverse(view) * gs_atom_position_cam).xyz)), 1.0);
    */
}
