#version 430

in flat int passInstanceID;
out vec4 fragColor;

//coherent layout(rgba16f, binding = 0) uniform uimageBuffer visibilityBuffer;
layout(rgba16f) uniform image1D visibilityBuffer;

uniform sampler2D tex;
uniform int level = 0;

void main() {
        float c = (texelFetch(tex, ivec2(gl_FragCoord), level).r);
        //int cc = int(c*1000);
        imageStore(visibilityBuffer, int(c), uvec4(1)); //write anything to detect non-zero locations
        fragColor = vec4(c);
}
