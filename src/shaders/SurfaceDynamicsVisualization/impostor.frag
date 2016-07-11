#version 430

in vec2 uv;
flat in float radius;
flat in vec3 center;
flat in vec3 color;
layout(location = 0) out vec3 fragColor;
layout (depth_less) out float gl_FragDepth; // Makes optimizations possible

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraWorldPos;
uniform vec3 lightDir;
uniform float clippingPlane;
uniform float depthDarkeningStart;
uniform float depthDarkeningEnd;

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
    vec3 worldPos = center + relativeWorldPos;

    // Cut at given clipping plane (could be optimized to use less ifs...)
    vec4 viewPos = view * vec4(worldPos, 1);
    float specularMultiplier = 1;
    bool isClippingPlane = false;
    if(-viewPos.z < clippingPlane)
    {
        // Check, whether back fragment on sphere is not inside clipping plane
        vec4 viewCenter = view * vec4(center, 1);
        if(-((2 * (viewCenter.z - viewPos.z)) + viewCenter.z) >= clippingPlane)
        {
            // Change view space normal
            normal = vec3(0,0,1);

            // Remember being clipping plane
            isClippingPlane = true;
        }
        else
        {
            // It is inside clipping plane, so discard it completely since fragment is not used to visualize clipping plane
            discard;
        }
    }

    // Set depth of pixel by projecting pixel position into clip space
    if(isClippingPlane)
    {
        gl_FragDepth = -0.5f;
    }
    else
    {
        vec4 projPos = projection * view * vec4(worldPos, 1.0);
        float projDepth = projPos.z / projPos.w;
        gl_FragDepth = (projDepth + 1.0) * 0.5; // gl_FragCoord.z is from 0..1. So go from clip space to viewport space
    }

    // Depth darkening
    float depthDarkening = (-viewPos.z - depthDarkeningStart) / (depthDarkeningEnd - depthDarkeningStart);
    depthDarkening = 1.0 - clamp(depthDarkening, 0, 1);

    // Diffuse lighting (hacked together, not correct)
    vec4 nrmLightDirection = normalize(vec4(lightDir, 0));
    float lighting = max(0,dot(normal, (view * -nrmLightDirection).xyz)); // Do it in view space (therefore is normal here ok)

    // Specular lighting (camera pos in view matrix last column is in view coordinates?)
    vec3 reflectionVector = reflect(nrmLightDirection.xyz, worldNormal);
    vec3 surfaceToCamera = normalize(cameraWorldPos - worldPos);
    float cosAngle = max(0.0, dot(surfaceToCamera, reflectionVector));
    float specular = pow(cosAngle, 10);
    if(isClippingPlane)
    {
        specular *= 0;
    }
    else
    {
        specular *= 0.5 * lighting;
    }

    // Some "ambient" lighting combined with specular
    vec3 finalColor = depthDarkening * mix(color * mix(vec3(0.4, 0.45, 0.5), vec3(1.0, 1.0, 1.0), lighting), vec3(1,1,1), specular);

    // Output color
    fragColor = finalColor;
}
