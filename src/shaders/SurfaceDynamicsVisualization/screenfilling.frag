#version 430

in vec2 uv;
layout(location = 0) out vec4 fragColor;
uniform sampler2D composite;
uniform sampler2D outline;

// Main function
void main()
{
    vec3 compositeColor = texture(composite, uv).rgb;
    vec4 outlineColor = texture(outline, uv);
    vec3 color = mix(compositeColor, outlineColor.rgb, outlineColor.a);
    fragColor = vec4(color, 1);
}
