//============================================================================
// Distributed under the MIT License. Author: Adrian Derstroff
//============================================================================

#version 120

in vec4 pos;

void main() {
    float inverseZ = (pos.z) / 1000.f;
	gl_FragColor = vec4(1.0, 1.0, 1.0, max(inverseZ-0.1, 0));
}
