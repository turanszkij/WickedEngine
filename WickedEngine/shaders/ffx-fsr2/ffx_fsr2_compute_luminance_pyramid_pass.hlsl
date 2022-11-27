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

#define FFX_HALF 0 // doesn't compile if enabled
#include "ShaderInterop_FSR2.h"

#define FSR2_BIND_SRV_INPUT_COLOR                     0
#define FSR2_BIND_UAV_SPD_GLOBAL_ATOMIC               0
#define FSR2_BIND_UAV_EXPOSURE_MIP_LUMA_CHANGE        1
#define FSR2_BIND_UAV_EXPOSURE_MIP_5                  2
#define FSR2_BIND_UAV_EXPOSURE                        3
#define FSR2_BIND_CB_FSR2                             0
#define FSR2_BIND_CB_SPD                              1

#include "ffx_fsr2_callbacks_hlsl.h"
#include "ffx_fsr2_common.h"

#if defined(FSR2_BIND_CB_SPD)
    cbuffer cbSPD : FFX_FSR2_DECLARE_CB(FSR2_BIND_CB_SPD) {

        uint    mips;
        uint    numWorkGroups;
        uint2   workGroupOffset;
        uint2   renderSize;
    };

    uint MipCount()
    {
        return mips;
    }

    uint NumWorkGroups()
    {
        return numWorkGroups;
    }

    uint2 WorkGroupOffset()
    {
        return workGroupOffset;
    }

    uint2 SPD_RenderSize()
    {
        return renderSize;
    }
#else
    uint MipCount()
    {
        return 0;
    }

    uint NumWorkGroups()
    {
        return 0;
    }

    uint2 WorkGroupOffset()
    {
        return uint2(0, 0);
    }

    uint2 SPD_RenderSize()
    {
        return uint2(0, 0);
    }
#endif


float2 SPD_LoadExposureBuffer()
{
#if defined(FSR2_BIND_UAV_EXPOSURE) || defined(FFX_INTERNAL)
    return rw_exposure[min16int2(0,0)];
#else
    return 0;
#endif
}

void SPD_SetExposureBuffer(float2 value)
{
#if defined(FSR2_BIND_UAV_EXPOSURE) || defined(FFX_INTERNAL)
    rw_exposure[min16int2(0,0)] = value;
#endif
}

float4 SPD_LoadMipmap5(int2 iPxPos)
{
#if defined(FSR2_BIND_UAV_EXPOSURE_MIP_5) || defined(FFX_INTERNAL)
    return float4(rw_img_mip_5[iPxPos], 0, 0, 0);
#else 
    return 0;
#endif
}

void SPD_SetMipmap(int2 iPxPos, int slice, float value)
{
    switch (slice)
    {
#if defined(FSR2_BIND_UAV_EXPOSURE_MIP_LUMA_CHANGE) || defined(FFX_INTERNAL)
    case FFX_FSR2_SHADING_CHANGE_MIP_LEVEL:
        rw_img_mip_shading_change[iPxPos] = value;
        break;
#endif
#if defined(FSR2_BIND_UAV_EXPOSURE_MIP_5) || defined(FFX_INTERNAL)
    case 5:
        rw_img_mip_5[iPxPos] = value;
        break;
#endif
    default:
        // avoid flattened side effect
#if defined(FSR2_BIND_UAV_EXPOSURE_MIP_LUMA_CHANGE) || defined(FFX_INTERNAL)
        rw_img_mip_shading_change[iPxPos] = rw_img_mip_shading_change[iPxPos];
#elif defined(FSR2_BIND_UAV_EXPOSURE_MIP_5) || defined(FFX_INTERNAL)
        rw_img_mip_5[iPxPos] = rw_img_mip_5[iPxPos];
#endif
        break;
    }
}

void SPD_IncreaseAtomicCounter(inout uint spdCounter)
{
   InterlockedAdd(rw_spd_global_atomic[min16int2(0,0)], 1, spdCounter);
}

void SPD_ResetAtomicCounter()
{
    rw_spd_global_atomic[min16int2(0,0)] = 0;
}

#include "ffx_fsr2_compute_luminance_pyramid.h"

#ifndef FFX_FSR2_THREAD_GROUP_WIDTH
#define FFX_FSR2_THREAD_GROUP_WIDTH 256
#endif // #ifndef FFX_FSR2_THREAD_GROUP_WIDTH
#ifndef FFX_FSR2_THREAD_GROUP_HEIGHT
#define FFX_FSR2_THREAD_GROUP_HEIGHT 1
#endif // #ifndef FFX_FSR2_THREAD_GROUP_HEIGHT
#ifndef FFX_FSR2_THREAD_GROUP_DEPTH
#define FFX_FSR2_THREAD_GROUP_DEPTH 1
#endif // #ifndef FFX_FSR2_THREAD_GROUP_DEPTH
#ifndef FFX_FSR2_NUM_THREADS
#define FFX_FSR2_NUM_THREADS [numthreads(FFX_FSR2_THREAD_GROUP_WIDTH, FFX_FSR2_THREAD_GROUP_HEIGHT, FFX_FSR2_THREAD_GROUP_DEPTH)]
#endif // #ifndef FFX_FSR2_NUM_THREADS

#include "globals.hlsli"

FFX_FSR2_NUM_THREADS
FFX_FSR2_EMBED_CB2_ROOTSIG_CONTENT
void main(uint3 WorkGroupId : SV_GroupID, uint LocalThreadIndex : SV_GroupIndex)
{
    ComputeAutoExposure(WorkGroupId, LocalThreadIndex);
}
