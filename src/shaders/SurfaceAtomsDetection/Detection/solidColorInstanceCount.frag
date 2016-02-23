#version 450

flat in int passInstanceID;

out vec4 fragColor;
out vec4 InstanceID;
in vec4 passColor;

void main() {
    fragColor = passColor;
    InstanceID = vec4(passInstanceID);
}
