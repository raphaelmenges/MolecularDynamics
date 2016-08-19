//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#version 430

// In / out
in vec2 uv;
layout(location = 0) out float fragColor;

// Uniforms
uniform int numDirections = 6;
uniform sampler2D depthTexture;

// Main function
void main()
{
    // TODO

    // Let the graphics card do some simple filtering
    fragColor = 0.5 < texture(depthTexture, uv).r ? 0.0 : 1.0;
}
