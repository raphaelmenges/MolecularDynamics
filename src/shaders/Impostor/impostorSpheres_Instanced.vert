#version 430

layout(location = 0) in vec4 positionAttribute;
layout(location = 1) in vec4 colorAttribute;
layout(location = 2) in vec4 instance_positionAttribute;

out vec2 texCoord;
out float depth;
out vec4 passPosition;
out float size;

flat out mat4 model;

out vec4 passColor;
out vec3 passWorldNormal;
out vec4 passWorldPosition;

flat out int passInstanceID;

uniform mat4 view;
uniform mat4 projection;
uniform vec2 scale;
uniform float elapsedTime;

void main() {
    // model size is found at instance_positionAttribute.w,
    // resize it according to input
    size = instance_positionAttribute.w * scale.x;

    // expected input vertices (positionAttribute) are a quad defined by [-1..1]²
    // position defines the center of the impostor geometry
    passPosition = view * vec4(instance_positionAttribute.xyz, 1) +
            positionAttribute * vec4(size, size, size, 1);

    // apply offset
    //int groupID = gl_InstanceID % 62;
    //passPosition = passPosition + vec4(sin(elapsedTime + groupID * 10),cos((elapsedTime + groupID * 10)/2),sin((elapsedTime + groupID * 10)/3),0);

    // for phong lighting shader
    passWorldPosition = passPosition;
    passWorldNormal = vec3(0,0,1);

    // fragment coordinates [-1..1]² are required in the fragment shader
    texCoord = positionAttribute.xy;

    // depth for manual depth buffer
    depth = -passPosition.z;

    gl_Position = projection * passPosition;

    // color has to be transferred to the fragment shader
    passColor = colorAttribute;

    // forward instanceID to FS
    passInstanceID = gl_InstanceID;

    model = mat4(1);
    model[3][0] = instance_positionAttribute.x;
    model[3][1] = instance_positionAttribute.y;
    model[3][2] = instance_positionAttribute.z;
}
