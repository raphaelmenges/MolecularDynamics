#version 450

uniform mat4  projection;
uniform float probe_radius;

flat in vec3  gs_torus_center_cam;
flat in float gs_torus_radius;
flat in vec3  gs_tangent1_center_cam;
flat in vec3  gs_tangent2_center_cam;

in vec3       gs_hit_position_cam;

out vec4      frag_color;
layout (depth_greater) out float gl_FragDepth;

const int     raymarch_steps_max = 128;
const float   raymarch_epsilon   = 0.0001;


mat3 rotation_matrix()
{
    vec3  origin_center_axis = vec3(0.0, 1.0, 0.0);
    vec3  torus_normal       = normalize(gs_tangent1_center_cam - gs_tangent2_center_cam);

    vec3  axis               = normalize(cross(origin_center_axis, torus_normal));
    float angle              = acos(dot(origin_center_axis, torus_normal));

    float s                  = sin(angle);
    float c                  = cos(angle);
    float oc                 = 1.0 - c;

    return mat3(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c);
}

bool is_outside_impostor_walls(vec3 position, float distance_torus_tangent1, float distance_torus_tangent2)
{
    return position.y >= distance_torus_tangent1 || position.y <= -distance_torus_tangent2;
}

bool is_outside_torus_radius(vec3 position)
{
    return length(position.xz) > gs_torus_radius;
}

float distance_cylinder(vec3 position)
{
    return length(position.xz) - gs_torus_radius;
}

float distance_torus(vec3 position)
{
    return length(vec2(length(position.xz) - gs_torus_radius, position.y)) - probe_radius;
}

float distance_to_surface(vec3 position)
{
    return max(-distance_torus(position), distance_cylinder(position));
}

void main()
{
    mat3  rotation                = rotation_matrix();

    vec3  hit_position_model      = rotation * (gs_hit_position_cam - gs_torus_center_cam);

    vec3  ray_direction_cam       = normalize(gs_hit_position_cam);
    vec3  ray_direction_model     = rotation * ray_direction_cam;

    float distance_torus_tangent1 = length(gs_tangent1_center_cam - gs_torus_center_cam);
    float distance_torus_tangent2 = length(gs_tangent2_center_cam - gs_torus_center_cam);

    float distance                = distance_to_surface(hit_position_model);

    vec3  real_hit_position_model = hit_position_model;

    for (int step = 0; step < raymarch_steps_max; step++)
    {
        real_hit_position_model += ray_direction_model * distance;

        distance = distance_to_surface(real_hit_position_model);

        if (abs(distance) < raymarch_epsilon) break;

        if (step == 0 && is_outside_impostor_walls(real_hit_position_model, distance_torus_tangent1, distance_torus_tangent2)) discard;
    }

    if (is_outside_impostor_walls(real_hit_position_model, distance_torus_tangent1, distance_torus_tangent2)) discard;

    // not needed
    //if (is_outside_torus_radius(real_hit_position_model)) discard;

    vec3 real_hit_position_cam  = gs_torus_center_cam + inverse(rotation) * real_hit_position_model;
    vec3 circle_direction_model = normalize(vec3(real_hit_position_model.x, 0.0, real_hit_position_model.z));
    vec3 circle_direction_cam   = inverse(rotation) * circle_direction_model;
    vec3 circle_position_cam    = gs_torus_center_cam + gs_torus_radius * circle_direction_cam;
    vec3 normal                 = normalize(circle_position_cam - real_hit_position_cam);

    frag_color = vec4(abs(normal), 1.0);

    vec4 real_hit_position_screen = projection * vec4(real_hit_position_cam, 1.0);
    gl_FragDepth = 0.5 * (real_hit_position_screen.z / real_hit_position_screen.w) + 0.5;
}

