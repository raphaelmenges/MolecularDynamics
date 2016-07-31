#version 430

layout(points) in;
layout(points, max_vertices = 1) out;

in vec3 vertColor[1];
flat out vec3 col;

uniform mat4 projection;
uniform mat4 view;
uniform float clippingPlane;

void main()
{
    // Decide whether to render sample
    float dist = -(view * gl_in[0].gl_Position).z;
    float halfRenderWidth = 0.25f;
    if((clippingPlane == 0) || (dist >= clippingPlane - halfRenderWidth && dist <= clippingPlane + halfRenderWidth)) // TODO think about less hardcoded
    {
        // Combine matrices
        mat4 M = projection * view;

        // Single point
        gl_Position = M * gl_in[0].gl_Position;
        col = vertColor[0];
        EmitVertex();

        EndPrimitive();
    }
}
