//============================================================================
// Distributed under the MIT License. Author: Adrian Derstroff
//============================================================================

#version 430

layout(location = 0) in vec4 vertex_position;

uniform mat4 viewMat;
uniform mat4 projMat;

out vec4 pos;

void main() {
	gl_Position = projMat * viewMat * vertex_position;
	pos = gl_Position;
}
