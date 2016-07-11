#version 430 core

// In / out
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;
smooth out vec3 eyeDirection;

// Uniforms
uniform mat4 view;
uniform mat4 projection;

// Global variables
vec4 vertPositions[] = {vec4(1, 1, 0, 1), vec4(-1, 1, 0, 1), vec4(1, -1, 0, 1), vec4(-1, -1, 0, 1)};

// Main function
void main()
{
    // http://gamedev.stackexchange.com/questions/60313/implementing-a-skybox-with-glsl-version-330
    mat4 inverseProjection = inverse(projection);
    mat3 inverseModelview = transpose(mat3(view));
    vec3 unprojected;

    // Emit vertices
    for(int i = 0; i < 4; i++)
    {
        unprojected = (inverseProjection * vertPositions[i]).xyz;
        eyeDirection = inverseModelview * unprojected;
        gl_Position = vertPositions[i];
        EmitVertex();
    }

    // End the quad
    EndPrimitive();
}
