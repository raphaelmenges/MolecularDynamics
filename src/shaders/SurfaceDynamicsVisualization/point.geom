//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#version 430

layout(points) in;

layout(points, max_vertices = 1) out;

uniform mat4 projection;
uniform mat4 view;
uniform float clippingPlane;

void main()
{
    vec3 center = gl_in[0].gl_Position.xyz;

    // Decide whether to render point
    if(-(view * gl_in[0].gl_Position).z >= clippingPlane)
    {
        // Combine matrices
        mat4 M = projection * view;

        // Single point
        gl_Position = M * vec4(center, 1);
        EmitVertex();

        EndPrimitive();
    }
}
