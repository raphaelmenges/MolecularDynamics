#version 450

layout(location = 0) in vec4 positionAttribute;

layout(std430, binding = 0) buffer instance_positions_t
{
vec4 instance_positions[];
};
layout(std430, binding = 1) buffer instance_colors_t
{
vec4 instance_colors[];
};

layout(binding = 1, r32ui) uniform uimage1D visibleIDsBuff;

out vec2 texCoord;
out float depth;
out vec4 passPosition;
out float size;

flat out mat4 model;

out vec4 passColor;
out vec3 passWorldNormal;
out vec4 passWorldPosition;

flat out int passInstanceID;
flat out vec3 center;

uniform mat4 view;
uniform mat4 projection;
uniform vec2 scale;
uniform float elapsedTime;
uniform float probeRadius = 0.0;

void main() {

    // read which sphere ID this instance will draw
    uint sphereID = imageLoad(visibleIDsBuff, gl_InstanceID).r;

    // model size is found at instance_positionAttribute.w,
    // resize it according to input
    size = instance_positions[sphereID].w * scale.x + probeRadius;

    // expected input vertices (positionAttribute) are a quad defined by [-1..1]²
    // position defines the center of the impostor geometry
    passPosition = view * vec4(instance_positions[sphereID].xyz, 1) +
    positionAttribute * vec4(size, size, size, 1);

    // apply offset
    int groupID = gl_InstanceID % 62;
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
    passColor = instance_colors[sphereID];

    // forward instanceID to FS
    passInstanceID = int(sphereID);

    model = mat4(1);
    model[3][0] = instance_positions[sphereID].x;
    model[3][1] = instance_positions[sphereID].y;
    model[3][2] = instance_positions[sphereID].z;
    //model[3][3] = 1;
   center = instance_positions[sphereID].xyz;

}
