#version 430

// In / out
in vec3 position;
out flat vec3 color;

// Uniforms
uniform mat4 projection;
uniform mat4 view;
uniform vec3 pastColor;
uniform vec3 futureColor;
uniform int frame;
uniform int offset;

// Main function
void main()
{
    gl_Position = projection * view * vec4(position, 1);

    // TODO: very simple test. In future, maybe start frame is necessary
    // when only part of trajectory is in path and gl_VertexID is relative
    if(int((gl_VertexID) + offset) <= frame)
    {
        color = pastColor;
    }
    else
    {
        color = futureColor;
    }
}
