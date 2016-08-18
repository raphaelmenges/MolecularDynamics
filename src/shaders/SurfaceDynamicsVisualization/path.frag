//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#version 430

// In / out
layout(location = 0) out vec4 fragColor;
in flat vec4 color;

// Main function
void main()
{
    fragColor = color;
}
