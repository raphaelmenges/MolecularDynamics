#version 430

layout(points) in;

layout(triangle_strip, max_vertices = 4) out;

in vec3 vertColor[1];
in float vertRadius[1];
out vec2 uv;
flat out float radius;
flat out vec3 center;
flat out vec3 color;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    // Get center
    center = gl_in[0].gl_Position.xyz;

    // Set color and radius which is for all vertices the same
    color = vertColor[0];
    radius = vertRadius[0];

    // GLSL is column-major! Get world space camera vectors
    vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]); // First row of view matrix
    vec3 cameraUp = vec3(view[0][1], view[1][1], view[2][1]); // Second row of view matrix

    // Combine matrices
    mat4 M = projection * view;

    // Emit quad
    gl_Position = M * (gl_in[0].gl_Position + radius * vec4(cameraRight+cameraUp, 0));
    uv = vec2(1,1);
    EmitVertex();

    gl_Position = M * (gl_in[0].gl_Position + radius * vec4(-cameraRight+cameraUp, 0));
    uv = vec2(-1,1);
    EmitVertex();

    gl_Position = M * (gl_in[0].gl_Position + radius * vec4(cameraRight-cameraUp, 0));
    uv = vec2(1,-1);
    EmitVertex();

    gl_Position = M * (gl_in[0].gl_Position + radius * vec4(-cameraRight-cameraUp, 0));
    uv = vec2(-1,-1);
    EmitVertex();

    EndPrimitive();
}
