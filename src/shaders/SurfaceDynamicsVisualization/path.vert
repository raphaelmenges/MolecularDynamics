//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#version 430

// In / out
in vec3 position;
out flat vec4 color;

// Uniforms
uniform mat4 projection;
uniform mat4 view;
uniform vec3 pastColor;
uniform vec3 futureColor;
uniform int frame;
uniform int offset;
uniform int frameRadius;

// Main function
void main()
{
    // Set position
    gl_Position = projection * view * vec4(position, 1);

    // Extract frame for that position
    int thisFrame = int(gl_VertexID) + offset;
    int frameDelta = abs(frame - thisFrame);
    float alpha = 1.0 - (0.5 * float(frameDelta) / float(frameRadius));

    // Set color
    if(thisFrame <= frame)
    {
        color = vec4(pastColor, alpha);
    }
    else
    {
        color = vec4(futureColor, alpha);
    }
}
