//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#version 430

in vec2 uv;
layout(location = 0) out vec4 fragColor;
uniform sampler2D molecule;
uniform sampler2D selectedAtom;
uniform sampler2D overlay;
uniform float moleculeAlpha;

// Main function
void main()
{
    vec4 moleculeColor = texture(molecule, uv);
    vec4 selectedAtomColor = texture(selectedAtom, uv);
    vec4 overlayColor = texture(overlay, uv);
    vec4 color = mix(vec4(0,0,0,0), moleculeColor, moleculeAlpha);
    color = mix(color, selectedAtomColor, 0.5 * selectedAtomColor.a);
    fragColor = mix(color, overlayColor, overlayColor.a);
}
