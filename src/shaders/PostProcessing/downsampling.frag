//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#version 430

in vec2 uv;
layout(location = 0) out float fragColor;
uniform sampler2D depthTexture;

// Main function
void main()
{
    // Let the graphics card do some simple linear filtering
    fragColor = texture(depthTexture, uv).r;
}
