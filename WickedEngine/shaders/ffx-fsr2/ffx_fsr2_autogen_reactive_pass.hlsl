// This file is part of the FidelityFX SDK.
//
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "ShaderInterop_FSR2.h"

#define FSR2_BIND_SRV_PRE_ALPHA_COLOR                       0
#define FSR2_BIND_SRV_POST_ALPHA_COLOR                      1
#define FSR2_BIND_UAV_REACTIVE                              0
#define FSR2_BIND_CB_FSR2                                   0

#include "ffx_fsr2_callbacks_hlsl.h"
#include "ffx_fsr2_common.h"

Texture2D<float4>                       r_input_color_pre_alpha             : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_PRE_ALPHA_COLOR);
Texture2D<float4>                       r_input_color_post_alpha            : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_POST_ALPHA_COLOR);
RWTexture2D<float>                      rw_output_reactive_mask             : FFX_FSR2_DECLARE_UAV(FSR2_BIND_UAV_REACTIVE);

#ifndef FFX_FSR2_THREAD_GROUP_WIDTH
#define FFX_FSR2_THREAD_GROUP_WIDTH 8
#endif // #ifndef FFX_FSR2_THREAD_GROUP_WIDTH
#ifndef FFX_FSR2_THREAD_GROUP_HEIGHT
#define FFX_FSR2_THREAD_GROUP_HEIGHT 8
#endif // FFX_FSR2_THREAD_GROUP_HEIGHT
#ifndef FFX_FSR2_THREAD_GROUP_DEPTH
#define FFX_FSR2_THREAD_GROUP_DEPTH 1
#endif // #ifndef FFX_FSR2_THREAD_GROUP_DEPTH
#ifndef FFX_FSR2_NUM_THREADS
#define FFX_FSR2_NUM_THREADS [numthreads(FFX_FSR2_THREAD_GROUP_WIDTH, FFX_FSR2_THREAD_GROUP_HEIGHT, FFX_FSR2_THREAD_GROUP_DEPTH)]
#endif // #ifndef FFX_FSR2_NUM_THREADS

cbuffer cbGenerateReactive : register(b0)
{
    float   scale;
    float   threshold;
    float   binaryValue;
    uint    flags;
};

#include "globals.hlsli"

FFX_FSR2_NUM_THREADS
FFX_FSR2_EMBED_ROOTSIG_CONTENT
void main(uint2 uGroupId : SV_GroupID, uint2 uGroupThreadId : SV_GroupThreadID)
{
    uint2 uDispatchThreadId = uGroupId * uint2(FFX_FSR2_THREAD_GROUP_WIDTH, FFX_FSR2_THREAD_GROUP_HEIGHT) + uGroupThreadId;

    float3 ColorPreAlpha    = r_input_color_pre_alpha[uDispatchThreadId].rgb;
    float3 ColorPostAlpha   = r_input_color_post_alpha[uDispatchThreadId].rgb;
    
    if (flags & FFX_FSR2_AUTOREACTIVEFLAGS_APPLY_TONEMAP)
    {
        ColorPreAlpha = Tonemap(ColorPreAlpha);
        ColorPostAlpha = Tonemap(ColorPostAlpha);
    }

    if (flags & FFX_FSR2_AUTOREACTIVEFLAGS_APPLY_INVERSETONEMAP)
    {
        ColorPreAlpha = InverseTonemap(ColorPreAlpha);
        ColorPostAlpha = InverseTonemap(ColorPostAlpha);
    }

    float out_reactive_value = 0.f;
    float3 delta = abs(ColorPostAlpha - ColorPreAlpha);
    
    out_reactive_value = (flags & FFX_FSR2_AUTOREACTIVEFLAGS_USE_COMPONENTS_MAX) ? max(delta.x, max(delta.y, delta.z)) : length(delta);
    out_reactive_value *= scale;

    out_reactive_value = (flags & FFX_FSR2_AUTOREACTIVEFLAGS_APPLY_THRESHOLD) ? (out_reactive_value < threshold ? 0 : binaryValue) : out_reactive_value;

    rw_output_reactive_mask[uDispatchThreadId] = out_reactive_value;
}
