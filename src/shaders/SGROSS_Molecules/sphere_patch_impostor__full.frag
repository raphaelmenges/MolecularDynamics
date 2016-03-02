#version 450

uniform mat4  projection;
uniform float probe_radius;

flat in vec3  gs_probe_position_cam;
flat in vec3  gs_atom1_position_cam;
flat in vec3  gs_atom2_position_cam;
flat in vec3  gs_atom3_position_cam;
in vec3       gs_hit_position_cam;

out vec4 frag_color;
layout (depth_greater) out float gl_FragDepth;

const vec3 n1 = normalize(cross(gs_atom2_position_cam.xyz - gs_probe_position_cam.xyz, gs_atom1_position_cam.xyz - gs_probe_position_cam.xyz));
const vec3 n2 = normalize(cross(gs_atom3_position_cam.xyz - gs_probe_position_cam.xyz, gs_atom2_position_cam.xyz - gs_probe_position_cam.xyz));
const vec3 n3 = normalize(cross(gs_atom1_position_cam.xyz - gs_probe_position_cam.xyz, gs_atom3_position_cam.xyz - gs_probe_position_cam.xyz));
const vec3 n4 = normalize(cross(gs_atom3_position_cam.xyz - gs_atom1_position_cam.xyz, gs_atom2_position_cam.xyz - gs_atom1_position_cam.xyz));

const vec3  light_direction_cam = normalize(vec3(-1.0, 1.0, 1.0));
const vec3  atom_color_diffuse  = vec3(0.8, 0.8, 0.8);
const vec3  atom_color_ambient  = vec3(0.2, 0.2, 0.2);
const vec3  atom_color_specular = vec3(0.3, 0.3, 0.3);


bool check_position_in_tetrahedron(vec3 position)
{
    vec3 p = position - gs_probe_position_cam.xyz;

    return dot(n1, p) > 0 && dot(n2, p) > 0 && dot(n3, p) > 0 && dot(n4, p) > 0;
    /*
    vec4 v0 = vec4((inverse(view) * gs_probe_position_cam).xyz, 1.0);
    vec4 v1 = vec4((inverse(view) * gs_atom1_position_cam).xyz, 1.0);
    vec4 v2 = vec4((inverse(view) * gs_atom2_position_cam).xyz, 1.0);
    vec4 v3 = vec4((inverse(view) * gs_atom3_position_cam).xyz, 1.0);


    vec4 p0 = inverse(view) * vec4(position, 1.0);

    float det0 = determinant(mat4(v0, v1, v2, v3));
    float det1 = determinant(mat4(p0, v1, v2, v3));
    float det2 = determinant(mat4(v0, p0, v2, v3));
    float det3 = determinant(mat4(v0, v1, p0, v3));
    float det4 = determinant(mat4(v0, v1, v2, p0));

    if (det0 != 0)
    {
        if (det0 < 0)
        {
            if ((det1 < 0) && (det2 < 0) && (det3 < 0) && (det4 < 0))
            {
                return true;
            }
        }
        if (det0 > 0)
        {
            if ((det1 > 0) && (det2 > 0) && (det3 > 0) && (det4 > 0))
            {
                return true;
            }
        }
    }
    return false;
    */
}

void main()
{
    vec3 ray_direction = normalize(gs_hit_position_cam);

    float a = dot(ray_direction, -gs_probe_position_cam);
    float b = a * a - length(gs_probe_position_cam) * length(gs_probe_position_cam) + probe_radius * probe_radius;

    if (b < 0) discard; // no intercections

    float d = -a + sqrt(b); // always draw the point in the back
    vec3 real_hit_position_cam = d * ray_direction;

    if (!check_position_in_tetrahedron(real_hit_position_cam))
    {
        discard;
    }

    vec3 normal = normalize(gs_probe_position_cam - real_hit_position_cam);

    frag_color = vec4(atom_color_ambient +
                      clamp(atom_color_diffuse * max(dot(normal, light_direction_cam), 0.0), 0.0, 1.0) +
                      clamp(atom_color_specular * pow(max(dot(reflect(-light_direction_cam, normal), ray_direction),0.0), 2.0), 0.0, 1.0), 1.0);

    vec4 real_position_screen = projection * vec4(real_hit_position_cam, 1.0);
    gl_FragDepth = 0.5 * (real_position_screen.z / real_position_screen.w) + 0.5;
}
