#version 430

in vec2 uv;
flat in float radius;
flat in vec3 position;
flat in vec3 color;
out vec4 outColor;
layout (depth_less) out float gl_FragDepth; // Makes optimizations possible

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraWorldPos;
uniform vec3 lightDir;
uniform vec3 minPosition;
uniform vec3 maxPosition;

void main()
{
    // Radius in UV space is 1 (therefore the scaling with 2 in geometry shader)

    // Distance from center of sphere
    float distance = length(uv);
    if(distance > 1.0)
    {
            discard;
    }

    // Calculate normal of sphere
    float z = sqrt(1.0 - dot(uv,uv)); // 1.0 -((uv.x*uv.x) + (uv.y*uv.y)));
    vec3 normal = normalize(vec3(uv, z));

    // World space position on sphere
    vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]); // First row of view matrix
    vec3 cameraUp = vec3(view[0][1], view[1][1], view[2][1]); // Second row of view matrix
    vec3 cameraDepth = vec3(view[0][2], view[1][2], view[2][2]); // Third row of view matrix
    vec3 relativeViewPos = normal * radius;
    vec3 relativeWorldPos = vec3(relativeViewPos.x * cameraRight + relativeViewPos.y * cameraUp + relativeViewPos.z * cameraDepth);
    vec3 worldNormal = normalize(relativeWorldPos);
    vec3 worldPos = position + relativeWorldPos;

    // TODO / TESTING (Note: one can already discard atoms which are completely hidden in geometry shader
    vec3 testColor = color;
    if(worldPos.x < minPosition.x)
    {
        worldPos.x = minPosition.x;
        if(abs(length(worldPos - position)) > radius)
        {
            discard;
        }
        testColor = vec3(0,0,1);
    }
    if(worldPos.x > maxPosition.x)
    {
        testColor = vec3(0,1,1);
    }

    // Set depth of pixel by projecting pixel position into clip space
    vec4 projPos = projection * view * vec4(worldPos, 1.0);
    float projDepth = projPos.z / projPos.w;
    gl_FragDepth = (projDepth + 1.0) * 0.5; // gl_FragCoord.z is from 0..1. So go from clip space to viewport space

    // Diffuse lighting (hacked together, not correct)
    vec4 nrmLightDirection = normalize(vec4(lightDir, 0));
    float lighting = max(0,dot(normal, vec3(view * -nrmLightDirection))); // Do it in view space (therefore is normal here ok)

    // Specular lighting (camera pos in view matrix last column is in view coordinates?)
    vec3 reflectionVector = reflect(nrmLightDirection.xyz, worldNormal);
    vec3 surfaceToCamera = normalize(cameraWorldPos - worldPos);
    float cosAngle = max(0.0, dot(surfaceToCamera, reflectionVector));
    float specular = pow(cosAngle, 10);
    specular *= 0.5 * lighting;

    // Some "ambient" lighting combined with specular
    vec3 finalColor = mix(testColor * mix(vec3(0.4, 0.45, 0.5), vec3(1.0, 1.0, 1.0), lighting), vec3(1,1,1), specular);

    // Output color
    outColor = vec4(finalColor, 1);
}
