#version 430

layout(points) in;

layout(points, max_vertices = 1) out;

in vec3 vertColor[1];
flat out vec3 color;

uniform mat4 projection;
uniform mat4 view;
uniform float clippingPlane;

void main()
{
    vec3 center = gl_in[0].gl_Position.xyz;

    // Decide whether to render point
    if(-(view * gl_in[0].gl_Position).z >= clippingPlane)
    {
        // Color
        color = vertColor[0];

        // Combine matrices
        mat4 M = projection * view;

        // Single point
        gl_Position = M * vec4(center, 1);
        EmitVertex();

        EndPrimitive();
    }
}
