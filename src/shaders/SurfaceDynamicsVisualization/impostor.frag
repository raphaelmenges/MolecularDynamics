//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#version 430

// In / out
in vec2 uv;
flat in float radius;
flat in vec3 center;
flat in vec3 color;
flat in int index;
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 pickIndex;
out float gl_FragDepth; // No information about less because may not be discared
                        // before checked whether atom is part of analysis group
                        // and whether fragment inserted in image for rendering
                        // group atoms on top of molecule.

// Group indicator
layout(std430, binding = 2) restrict readonly buffer GroupIndicatorBuffer
{
  float groupIndicator[];
};

// Group rendering texture as image
layout(binding = 3, rgba32f) restrict coherent uniform image2D GroupRenderingImage;

// Semaphore for group rendering
layout(binding = 4, r32ui) restrict coherent uniform uimageBuffer GroupRenderingSemaphore;

// Uniforms
uniform float time;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraWorldPos;
uniform vec3 lightDir;
uniform float clippingPlane;
uniform float depthDarkeningStart;
uniform float depthDarkeningEnd;
uniform float highlightMultiplier;
uniform vec4 highlightColor;
uniform int framebufferWidth;

// Main function
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
        gl_FragDepth = 0.0;
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

    // Rim lighting
    finalColor += ((0.75 * lighting) + 0.25) * pow(1.0 - dot(normal, vec3(0,0,1)), 3);

    // Highlight (TODO: is float precision enough to add time with frag coord?)
    float highlight = (sin((((gl_FragCoord.x + gl_FragCoord.y)* 3.14) / 8.0) +  (8.f * time)) + 1.0) / 2.0;
    finalColor = mix(finalColor, highlightColor.rgb, highlightColor.a * groupIndicator[index] * 0.5 * highlight * highlightMultiplier);

    // Output group rendering fragment
    if(groupIndicator[index] > 0)
    {
        // Pixel coordinate of current fragment
        ivec2 pixelCoordinate = ivec2(gl_FragCoord.x, gl_FragCoord.y);

        // Depth of current fragment
        float groupRenderingDepth = 1.0 - gl_FragDepth; // inverse depth since texture is initialized with zero and would be already at near plane

        // Get linear coordinate in semaphore
        int semaphoreCoordinate = (framebufferWidth * pixelCoordinate.y) + pixelCoordinate.x;

        // Lock current pixel using semaphore
        int iterations = 0;
        const int maxIterations = 10;
        while(iterations < maxIterations // do it until iteration limit is reached
            && 0 != imageAtomicCompSwap( // compare to original value
                GroupRenderingSemaphore, // semaphore image
                semaphoreCoordinate, // coordiante in image buffer
                0, // value which is compared to
                1)) // value which is written if comparsion is successful
        {
            iterations++;
        }

        // Only proceed if locked
        if(iterations < maxIterations)
        {
            // Fetch current depth value from image and decide whether to overwrite stored value
            float prevGoupRenderingDepth = float(imageLoad(GroupRenderingImage, pixelCoordinate).a);
            if(prevGoupRenderingDepth < groupRenderingDepth)
            {
               imageStore(GroupRenderingImage, pixelCoordinate, vec4(finalColor.rgb, groupRenderingDepth));
            }

            memoryBarrier(); // guarantee that value is written to image

            // Reset semaphore that next shader may access that pixel
            imageAtomicExchange(
                GroupRenderingSemaphore,
                semaphoreCoordinate,
                0);
        }
    }

    // Output color
    fragColor = vec4(finalColor, 1);

    // Output pickIndex
    int rawPickIndex = index + 1; // add one to distinguish from nothing
    int r = (rawPickIndex & 0x000000FF) >>  0;
    int g = (rawPickIndex & 0x0000FF00) >>  8;
    int b = (rawPickIndex & 0x00FF0000) >> 16;
    pickIndex = vec3(
        float(r) / 255.0,
        float(g) / 255.0,
        float(b) / 255.0);
}
