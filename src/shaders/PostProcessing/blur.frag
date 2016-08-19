//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#version 430

in vec2 uv;
layout(location = 0) out float fragColor;
uniform sampler2D ambientOcclusion;

// Main function
void main()
{
    // Let the graphics card do some simple filtering
    fragColor = texture(ambientOcclusion, uv).r;
}
