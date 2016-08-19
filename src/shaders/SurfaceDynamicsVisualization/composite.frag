//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#version 430

in vec2 uv;
layout(location = 0) out vec4 fragColor;
uniform sampler2D molecule;
uniform sampler2D selectedAtom;
uniform sampler2D overlay;
uniform sampler2D ambientOcclusion;

// Main function
void main()
{
    vec4 moleculeColor = texture(molecule, uv);
    vec4 selectedAtomColor = texture(selectedAtom, uv);
    vec4 overlayColor = texture(overlay, uv);
    vec4 color = mix(moleculeColor, selectedAtomColor, 0.5 * selectedAtomColor.a);
    fragColor = mix(color, overlayColor, overlayColor.a);
    fragColor = texture(ambientOcclusion, uv);
}
