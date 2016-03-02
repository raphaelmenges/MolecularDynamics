#version 450

flat in vec4  gs_atom_position_world;
flat in vec4  gs_atom_position_cam;
flat in float gs_atom_radius;
in vec2       gs_impostor_coord;

out vec4 frag_color;


void main()
{
    if (length(gs_impostor_coord.xy) > 1.0)
    {
        discard;
    }

    frag_color = vec4(normalize(abs(gs_atom_position_world.xyz)), 1.0);
}
