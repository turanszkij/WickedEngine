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

// FSR2 pass 6
// SRV  4 : m_Exposure                          : r_exposure
// SRV 19 : FSR2_InternalUpscaled1              : r_rcas_input
// UAV 18 : DisplayOutput                       : rw_upscaled_output
// CB   0 : cbFSR2
// CB   1 : cbRCAS

#include "ShaderInterop_FSR2.h"

#define FSR2_BIND_SRV_EXPOSURE              0
#define FSR2_BIND_SRV_RCAS_INPUT            1
#define FSR2_BIND_UAV_UPSCALED_OUTPUT       0
#define FSR2_BIND_CB_FSR2                   0
#define FSR2_BIND_CB_RCAS                   1

#include "ffx_fsr2_callbacks_hlsl.h"
#include "ffx_fsr2_common.h"

//Move to prototype shader!
#if defined(FSR2_BIND_CB_RCAS)
    cbuffer cbRCAS : FFX_FSR2_DECLARE_CB(FSR2_BIND_CB_RCAS)
    {
        uint4 rcasConfig;
    };

    uint4 RCASConfig()
    {
        return rcasConfig;
    }
#else
    uint4 RCASConfig()
    {
        return 0;
    }
#endif

#if FFX_HALF
float4 LoadRCAS_Input(FfxInt16x2 iPxPos)
{
    return r_rcas_input[iPxPos];
}
#else
float4 LoadRCAS_Input(FfxInt32x2 iPxPos)
{
    return r_rcas_input[iPxPos];
}
#endif

#include "ffx_fsr2_rcas.h"

#ifndef FFX_FSR2_THREAD_GROUP_WIDTH
#define FFX_FSR2_THREAD_GROUP_WIDTH 64
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
void main(uint3 LocalThreadId : SV_GroupThreadID, uint3 WorkGroupId : SV_GroupID, uint3 Dtid : SV_DispatchThreadID)
{
    RCAS(LocalThreadId, WorkGroupId, Dtid);
}
