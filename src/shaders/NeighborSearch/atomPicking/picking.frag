//============================================================================
// Distributed under the MIT License. Author: Adrian Derstroff
//============================================================================

#version 430

in vec2 uv;
flat in float radius;
flat in vec3  position;
flat in int   id;

out vec3 FragColor;
layout (depth_less) out float gl_FragDepth; // Makes optimizations possible

uniform mat4 view;
uniform mat4 projection;

void main()
{
    // Radius in UV space is 1 (therefore the scaling with 2 in geometry shader)

    /*
     * render only sphere on the triangle billboard
     */
    float distance = length(uv);
    if(distance > 1.0)
    {
        discard;
    }

    /*
     * Calculate normal of sphere
     * calculate dotproduct to get the depth
     * the further the uv coordinate is from the center of the sphere the further z is away
     */
    float z = sqrt(1.0 - dot(uv,uv)); // 1.0 -((uv.x*uv.x) + (uv.y*uv.y)));
    vec3 normal = normalize(vec3(uv, z));

    /*
     * World space position on sphere
     */
     // view matrix
    vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]); // First row of view matrix
    vec3 cameraUp = vec3(view[0][1], view[1][1], view[2][1]); // Second row of view matrix
    vec3 cameraDepth = vec3(view[0][2], view[1][2], view[2][2]); // Third row of view matrix
    // position from camera
    vec3 relativeViewPos = normal * radius;
    vec3 relativeWorldPos = vec3(relativeViewPos.x * cameraRight + relativeViewPos.y * cameraUp + relativeViewPos.z * cameraDepth);
    vec3 worldNormal = normalize(relativeWorldPos);
    // world positon = relative positon to camera + position of atom center in world space
    vec3 worldPos = position + relativeWorldPos;

    // Set depth of pixel by projecting pixel position into clip space
    vec4 projPos = projection * view * vec4(worldPos, 1.0);
    float projDepth = projPos.z / projPos.w;
    gl_FragDepth = (projDepth + 1.0) * 0.5; // gl_FragCoord.z is from 0..1. So go from clip space to viewport space

    // Output color
    FragColor = vec3(float(id), float(id), float(gl_PrimitiveID + 1));
}
