#version 430

layout(points) in;

layout(triangle_strip, max_vertices = 3) out;

in float vertRadius[1];
in vec4 pos[1];

out vec2 uv;
flat out float radius;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    /*
     * Get atom position and search radius
     */
    vec4 position = pos[0];
    radius = vertRadius[0];

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
    gl_Position = M * (position + radius * vec4(-2 * cameraRight - cameraUp, 0));
    uv = 2 * (vec2(-0.5, 0) - 0.5);
    EmitVertex();

    gl_Position =  M * (position + radius * vec4(2 * cameraRight - cameraUp, 0));
    uv = 2 * (vec2(1.5, 0) - 0.5);
    EmitVertex();

    gl_Position = M * (position + radius * vec4(3 * cameraUp, 0));
    uv = 2 * (vec2(0.5, 2) - 0.5);
    EmitVertex();

    EndPrimitive();
}
