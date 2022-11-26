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

// FSR2 pass 3
// SRV  7 : FSR2_ReconstructedPrevNearestDepth  : r_reconstructed_previous_nearest_depth
// SRV  8 : FSR2_DilatedVelocity                : r_dilated_motion_vectors
// SRV  9 : FSR2_DilatedDepth                   : r_dilatedDepth
// UAV 12 : FSR2_DepthClip                      : rw_depth_clip
// CB   0 : cbFSR2

#include "ShaderInterop_FSR2.h"

#define FSR2_BIND_SRV_RECONSTRUCTED_PREV_NEAREST_DEPTH      0
#define FSR2_BIND_SRV_DILATED_MOTION_VECTORS                1
#define FSR2_BIND_SRV_DILATED_DEPTH                         2
#define FSR2_BIND_UAV_DEPTH_CLIP                            0
#define FSR2_BIND_CB_FSR2                                   0

#include "ffx_fsr2_callbacks_hlsl.h"
#include "ffx_fsr2_common.h"
#include "ffx_fsr2_sample.h"
#include "ffx_fsr2_depth_clip.h"

#ifndef FFX_FSR2_THREAD_GROUP_WIDTH
#define FFX_FSR2_THREAD_GROUP_WIDTH 8
#endif // #ifndef FFX_FSR2_THREAD_GROUP_WIDTH
#ifndef FFX_FSR2_THREAD_GROUP_HEIGHT
#define FFX_FSR2_THREAD_GROUP_HEIGHT 8
#endif // #ifndef FFX_FSR2_THREAD_GROUP_HEIGHT
#ifndef FFX_FSR2_THREAD_GROUP_DEPTH
#define FFX_FSR2_THREAD_GROUP_DEPTH 1
#endif // #ifndef FFX_FSR2_THREAD_GROUP_DEPTH
#ifndef FFX_FSR2_NUM_THREADS
#define FFX_FSR2_NUM_THREADS [numthreads(FFX_FSR2_THREAD_GROUP_WIDTH, FFX_FSR2_THREAD_GROUP_HEIGHT, FFX_FSR2_THREAD_GROUP_DEPTH)]
#endif // #ifndef FFX_FSR2_NUM_THREADS

#include "globals.hlsli"

FFX_FSR2_PREFER_WAVE64
FFX_FSR2_NUM_THREADS
FFX_FSR2_EMBED_ROOTSIG_CONTENT
void main(
    int2 iGroupId : SV_GroupID,
    int2 iDispatchThreadId : SV_DispatchThreadID,
    int2 iGroupThreadId : SV_GroupThreadID,
    int iGroupIndex : SV_GroupIndex)
{
    DepthClip(iDispatchThreadId);
}
