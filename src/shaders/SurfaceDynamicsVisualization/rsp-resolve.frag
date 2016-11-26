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

// Output
out vec4 fragColor;

// Input images
layout(r32ui) readonly uniform uimage2D	image_counter;
layout(rg32f) readonly uniform image2DArray image_peel;

// Getter for pixel count in k-Buffer
uint getPixelFragCounter()
{
    return imageLoad (image_counter, ivec2(gl_FragCoord.xy)).r;
}

// Getter for pixel value in k-Buffer
vec4 getPixelFragValue (const int coord_z)
{
    return imageLoad (image_peel, ivec3(gl_FragCoord.xy, coord_z));
}

// Helper for sorting methods. Holds data to sort
vec2 fragments [LOCAL_SIZE];

// Insertion sort
void sort_insert(const int num)
{
    for (int j = 1; j < num; ++j)
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
void sort_shell(const int num)
{
    int inc = num >> 1;
    while (inc > 0)
    {
        for (int i = inc; i < num; ++i)
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
void sort(const int num)
{
    // Decide which implementation to use
    if(num <= INSERTION_VS_SHELL)
        sort_insert(num);
    else
        sort_shell(num);
}

// TODO: what is this for a function?
int setMaxFromGlobalArray(float Z)
{
    int  id;
    vec2 maxFR = vec2(-1.0f,0.0f);

    for(int i=0; i<LOCAL_SIZE_1n; i++)
    {
        float Zi = fragments[i].g;
        if(maxFR.g < Zi)
        {
            maxFR.r = i;
            maxFR.g = Zi;
        }
    }

    if(Z < maxFR.g)
    {
        id = int(maxFR.r);
        fragments[LOCAL_SIZE_1n] = vec2(fragments[id].r, maxFR.g);
    }
    else
        id = LOCAL_SIZE_1n;

    return id;
}


// Resolving of fragment color. Takes count of pixel values in k-Buffer for this fragment
vec4 resolve(int num)
{
    sort(num);
    vec4 C = vec4(0,0,0,1);
    for(int i = 0; i < num; i++)
    {
        vec4 fragment =  unpackUnorm4x8(floatBitsToUint(fragments[i].r));
        //vec4 fragment =  unpackUnorm4x8(floatBitsToUint(pack));
        //C.xyz = C.xyz + (1 - C.a) * fragment.xyz * fragment.a;
        //C.a = (1 - fragment.a) * C.a;
        //fragment = vec4(1,0,0,0.5);
        C.xyz 	= C.a * (fragment.a * fragment.xyz) + C.xyz;
        C.a 	= (1.0 - fragment.a) * C.a;
    }
    return vec4(C);
}

// Main function
void main(void)
{
    int counterTotal = int(getPixelFragCounter());
    if(counterTotal > 0)
    {
        if (counterTotal > HEAP_SIZE)
        {
            int  counterLocal = 0;
            fragments[HEAP_SIZE_1n].g = 1.0f;

            for(int C=0; C<counterTotal; C++)
            {
                vec2 peel_pointer = getPixelFragValue(C).rg;

                if(counterLocal < HEAP_SIZE_1n)
                    fragments [counterLocal++] = peel_pointer.rg;
                else if(peel_pointer.g < fragments [HEAP_SIZE_1n].g)
                    fragments [setMaxFromGlobalArray(peel_pointer.g)] = peel_pointer.rg;
            }
            counterTotal = HEAP_SIZE;
        }
        else
        {
            for(int i=0; i<counterTotal; i++)
            fragments[i] = getPixelFragValue(i).rg;
        }
        // fragments now contains an unsorted list of valid fragments
        // -> sort and blend!

        fragColor = resolve(counterTotal);
    }
    else
        discard;
}
