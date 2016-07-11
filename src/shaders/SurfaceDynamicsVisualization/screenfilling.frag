#version 430

in vec2 uv;
layout(location = 0) out vec4 fragColor;
uniform sampler2D tex;

// Main function
void main()
{
    fragColor = texture(tex, uv);
}
