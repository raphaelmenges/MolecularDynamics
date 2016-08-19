//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#version 430 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

out vec2 uv;

// Main function
void main()
{
    gl_Position = vec4(1.0, 1.0, 0.5, 1.0);
    uv = vec2(1.0, 1.0);
    EmitVertex();

    gl_Position = vec4(-1.0, 1.0, 0.5, 1.0);
    uv = vec2(0.0, 1.0);
    EmitVertex();

    gl_Position = vec4(1.0, -1.0, 0.5, 1.0);
    uv = vec2(1.0, 0.0);
    EmitVertex();

    gl_Position = vec4(-1.0, -1.0, 0.5, 1.0);
    uv = vec2(0.0, 0.0);
    EmitVertex();

    EndPrimitive();
}
