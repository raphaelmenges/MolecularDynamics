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

#version 450

// Defines
const int LAYER_COUNT = 32;
const int LAYER_COUNT_MINUS_ONE = LAYER_COUNT - 1;
const int INSERTION_VS_SHELL = 16;

// Helper for sorting methods. Holds data to sort
vec2 fragments [LAYER_COUNT];

// Output
out vec4 fragColor;

// Input images
layout(r32ui, binding = 0) readonly restrict uniform uimage2D KBufferCounter;
layout(rg32f, binding = 1) readonly restrict uniform image2DArray KBuffer;

// Getter for pixel count in k-Buffer
uint getPixelFragCounter()
{
    return imageLoad(KBufferCounter, ivec2(gl_FragCoord.xy)).r;
}

// Getter for pixel value in k-Buffer
vec4 getPixelFragValue(const int coord_z)
{
    return imageLoad(KBuffer, ivec3(gl_FragCoord.xy, coord_z));
}

// Insertion sort
void sort_insert(const int count)
{
    for (int j = 1; j < count; ++j)
    {
        vec2 key = fragments[j];
        int i = j - 1;

        while (i >= 0 && fragments[i].g > key.g)
        {
            fragments[i+1] = fragments[i];
            --i;
        }
        fragments[i+1] = key;
    }
}

// Shell sort
void sort_shell(const int count)
{
    int inc = count >> 1;
    while (inc > 0)
    {
        for (int i = inc; i < count; ++i)
        {
            vec2 tmp = fragments[i];

            int j = i;
            while (j >= inc && fragments[j - inc].g > tmp.g)
            {
                fragments[j] = fragments[j - inc];
                j -= inc;
            }
            fragments[j] = tmp;
        }
        inc = int(inc / 2.2f + 0.5f);
    }
}

// Abstract sort
void sort(const int count)
{
    // Decide which implementation to use
    if(count <= INSERTION_VS_SHELL)
    {
        sort_insert(count);
    }
    else
    {
        sort_shell(count);
    }
}

// Resolving of fragment color. Takes count of pixel values in k-Buffer for this fragment
vec4 resolve(const int count)
{
    // Sort
    sort(count);

    // Blend
    vec4 C = vec4(0,0,0,1);
    for(int i = 0; i < count; i++)
    {
        vec4 value =  unpackUnorm4x8(floatBitsToUint(fragments[i].r)); // unpack float back to 32bit rgba
        C.xyz = C.a * (value.a * value.xyz) + C.xyz; // blend color
        C.a = (1.0 - value.a) * C.a; // blend alpha
    }

    // Return
    return C;
}

// Main function
void main(void)
{
    // Fetch count of pixel values from counter image
    int count = min(LAYER_COUNT, int(getPixelFragCounter())); // if there are more pixels than k-Buffer layers, ...yeah
    if(count > 0)
    {
        // Write fragments to local array for resolving
        for(int i = 0; i < count; i++)
        {
            fragments[i] = getPixelFragValue(i).rg;
        }

        // Do resolving and receive color
        vec4 color = resolve(count);
        fragColor = vec4(color.rgb, 1.0 - color.a);
    }
    else // not a single pixel value available for this fragment
    {
        discard;
    }
}
