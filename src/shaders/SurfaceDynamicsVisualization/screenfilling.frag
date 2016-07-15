#version 430

in vec2 uv;
layout(location = 0) out vec4 fragColor;
uniform sampler2D molecule;
uniform sampler2D overlay;

// Main function
void main()
{
    vec4 moleculeColor = texture(molecule, uv);
    vec4 overlayColor = texture(overlay, uv);
    fragColor = mix(moleculeColor, overlayColor, overlayColor.a);
}
