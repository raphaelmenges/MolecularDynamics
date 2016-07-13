#version 430

// In / out
in vec2 uv;
flat in float radius; // not used but given by impostor.geom
flat in vec3 center; // not used but given by impostor.geom
flat in vec3 color; // not used but given by impostor.geom
flat in int index; // not used but given by impostor.geom
layout(location = 0) out vec4 fragColor;

// Uniforms
uniform mat4 view;
uniform mat4 projection;
uniform vec4 outlineColor;

// Main function
void main()
{
    // Radius in UV space is 1 (therefore the scaling with 2 in geometry shader)

    // Distance from center of sphere
    float distance = length(uv);
    if(distance > 1.0)
    {
            discard;
    }

    // Output color
    fragColor = outlineColor;

}
