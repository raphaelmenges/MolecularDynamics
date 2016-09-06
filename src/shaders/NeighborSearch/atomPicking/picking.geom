#version 430

layout(points) in;
layout(triangle_strip, max_vertices = 3) out;

in float vertRadius[1];
in int   atomID[1];

out vec2 uv;
flat out float radius;
flat out vec3 position;
flat out int  id;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    /*
     * Get atom position
     */
    position = gl_in[0].gl_Position.xyz;

    /*
     * Set radius and atom id which is for all vertices the same
     */
    radius = vertRadius[0];
    id     = atomID[0];

    /*
     * GLSL is column-major! Get world space camera vectors
     */
    vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]); // First row of view matrix
    vec3 cameraUp = vec3(view[0][1], view[1][1], view[2][1]); // Second row of view matrix

    /*
     * combine camera view and projection matrices
     */
    mat4 M = projection * view;

    /*
     * create triangle billboard
     */
    gl_Position = M * (gl_in[0].gl_Position + radius * vec4(-2 * cameraRight - cameraUp, 0));
    uv = 2 * (vec2(-0.5, 0) - 0.5);
    EmitVertex();

    gl_Position =  M * (gl_in[0].gl_Position + radius * vec4(2 * cameraRight - cameraUp, 0));
    uv = 2 * (vec2(1.5, 0) - 0.5);
    EmitVertex();

    gl_Position = M * (gl_in[0].gl_Position + radius * vec4(3 * cameraUp, 0));
    uv = 2 * (vec2(0.5, 2) - 0.5);
    EmitVertex();

    EndPrimitive();
}
