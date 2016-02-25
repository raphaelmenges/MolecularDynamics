#version 450

in flat int passInstanceID;
out vec4 fragColor;

//coherent layout(rgba16f, binding = 0) uniform uimageBuffer collectedIDsBuffer;
layout(binding = 0, r8ui) uniform uimage1D collectedIDsBuffer;

uniform sampler2D tex;
uniform int level = 0;

void main() {
        float c = (texelFetch(tex, ivec2(gl_FragCoord), level).r);
        if (c < 0)
            discard;
        //int cc = int(c*1000);
        imageStore(collectedIDsBuffer, int(c), uvec4(1)); //write anything to detect non-zero locations
        fragColor = vec4(c);
}
