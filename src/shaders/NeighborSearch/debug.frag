#version 430

in vec2 texcoord;

uniform int totalNumElements;
uniform int numCells;
uniform int width;
uniform int height;

layout(std430, binding = 3) restrict readonly buffer GridcountBuffer   { int  gridcnt[]; };

void main() {
    int xPos = int(width*texcoord.x);
    int yPos = int(width*height*texcoord.y);
    int i = (yPos + xPos) / 100;
    float value = 0.f;
    if (i < numCells) {
        int numElements = gridcnt[i];
        value = (numElements > 0) ? numElements / totalNumElements : 0;
        //value = (i % 2 == 0) ? 0.5 : 0.3;
    } else {
        value = 1.f;
    }
	gl_FragColor = vec4(value, value, value, 1);
}