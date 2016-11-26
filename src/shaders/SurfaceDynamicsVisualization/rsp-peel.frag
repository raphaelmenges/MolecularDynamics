// Adapted by Raphael Menges
// Most parts taken from following software:
//
//  Software is distributed under the following BSD-style license:
//
//  Authors: Andreas A. Vasilakis - Georgios Papaioannou - Ioannis Fudos
//  Emails : abasilak@aueb.gr - gepap@aueb.gr - fudos@cs.uoi.gr
//
//  Copyright � 2015. Athens University of Economics & Business, All Rights Reserved.
//
//  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the
//	documentation and/or other materials provided with the distribution.
//
//  3. The name of the author may not be used to endorse or promote products derived from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
//  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
//  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
//  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//	This research has been co-financed by the European Union (European Social Fund - ESF) and Greek
//	national funds through the Operational Program ``Education and Lifelong Learning'' of the National
//	Strategic Reference Framework (NSRF) - Research Funding Program: ARISTEIA II-GLIDE (grant no.3712).
//
//--------------------------------------------------------------------------------------

// Does NOT support following features
// - Generating of pick index, so no mouse picking possible
// - Drawing into group rendering image

#version 450

// Defines (no idea whether necessary)
#define INTEL_ordering		0
#define NV_interlock		0

#define packing                 0
#define multipass               0

#define MAX_ITERATIONS          200

#define HEAP_SIZE               32
#define HEAP_SIZE_1p		HEAP_SIZE + 1
#define HEAP_SIZE_1n		HEAP_SIZE - 1
#define HEAP_SIZE_2d		HEAP_SIZE >> 1
#define HEAP_SIZE_LOG2		log2(HEAP_SIZE)
#define ARRAY_VS_HEAP		16
#define INSERTION_VS_SHELL	16

#define KB_SIZE                 8
#define STENCIL_SIZE		((HEAP_SIZE < 32) ? HEAP_SIZE : 32)

#define HISTOGRAM_SIZE		1024
#define LOCAL_SIZE              32
#define LOCAL_SIZE_1n		LOCAL_SIZE - 1

#define Packed_1f               4294967295U // 0xFFFFFFFFU

// Output images
layout(binding = 2, r32ui) restrict coherent uniform uimage2D KBufferCounter;
layout(binding = 3, rg32f) writeonly restrict uniform image2DArray KBuffer;

// In / out
in vec2 uv;
flat in float radius;
flat in vec3 center;
flat in vec3 color;
flat in int index;
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 pickIndex;

// Uniforms
uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraWorldPos;
uniform vec3 lightDir;
uniform float clippingPlane;
uniform float depthDarkeningStart;
uniform float depthDarkeningEnd;

// Incremention of counter which counts pixels in k-Buffer
uint addPixelFragCounter()
{
    return imageAtomicAdd(KBufferCounter, ivec2(gl_FragCoord.xy), 1U);
}

// Sets fragment in key buffer
void setPixelFragValue(const int coord_z, const vec4 val)
{
    // Image array interpeted as 3D structure
    imageStore(KBuffer, ivec3(gl_FragCoord.xy, coord_z), val);
}

// Submit pixel value to k-Buffer
void submitPixelFragValueToOIT(vec4 val, float zValue)
{
    float C = uintBitsToFloat(packUnorm4x8(vec4(val))); // pack 32bit RGBA into single float
    int index = int(addPixelFragCounter()); // increment pixel counter for processed fragment and get valid k-Buffer index for current value
    setPixelFragValue(index, vec4(C, zValue, 0.0f, 0.0f)); // add value to k-Buffer

    // TODO: Testing
    fragColor = vec4(float((index) * 10) / 255.0, 0.0, 0.0, 1.0);
}

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
    float customDepth = 0.0;
    if(!isClippingPlane)
    {
        vec4 projPos = projection * view * vec4(worldPos, 1.0);
        float projDepth = projPos.z / projPos.w;
        customDepth = (projDepth + 1.0) * 0.5; // gl_FragCoord.z is from 0..1. So go from clip space to viewport space
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

    // Output color into k-Buffer
    submitPixelFragValueToOIT(vec4(finalColor, 0.75), customDepth);
    // fragColor = vec4(finalColor, 1); // rendering it to framebuffer, too (TODO: delete)

    // Set pick index to nothing
    pickIndex = vec3(0, 0, 0);
}

