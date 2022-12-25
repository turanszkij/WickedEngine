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

// FSR2 pass 5
// SRV  4 : FSR2_Exposure                       : r_exposure
// SRV  6 : m_UpscaleTransparencyAndComposition : r_transparency_and_composition_mask
// SRV  8 : FSR2_DilatedVelocity                : r_dilated_motion_vectors
// SRV 10 : FSR2_InternalUpscaled2              : r_internal_upscaled_color
// SRV 11 : FSR2_LockStatus2                    : r_lock_status
// SRV 12 : FSR2_DepthClip                      : r_depth_clip
// SRV 13 : FSR2_PreparedInputColor             : r_prepared_input_color
// SRV 14 : FSR2_LumaHistory                    : r_luma_history
// SRV 16 : FSR2_LanczosLutData                 : r_lanczos_lut
// SRV 26 : FSR2_MaximumUpsampleBias            : r_upsample_maximum_bias_lut
// SRV 27 : FSR2_DilatedReactiveMasks           : r_dilated_reactive_masks
// SRV 28 : FSR2_ExposureMips                   : r_imgMips
// UAV 10 : FSR2_InternalUpscaled1              : rw_internal_upscaled_color
// UAV 11 : FSR2_LockStatus1                    : rw_lock_status
// UAV 18 : DisplayOutput                       : rw_upscaled_output
// CB   0 : cbFSR2
// CB   1 : FSR2DispatchOffsets

#include "ShaderInterop_FSR2.h"

#pragma dxc diagnostic push
#pragma dxc diagnostic ignored "-Wfor-redefinition"

#define FSR2_BIND_SRV_EXPOSURE                               0
#if FFX_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS
#define FSR2_BIND_SRV_DILATED_MOTION_VECTORS                 2
#else
#define FSR2_BIND_SRV_MOTION_VECTORS                         2
#endif
#define FSR2_BIND_SRV_INTERNAL_UPSCALED                      3
#define FSR2_BIND_SRV_LOCK_STATUS                            4
#define FSR2_BIND_SRV_DEPTH_CLIP                             5
#define FSR2_BIND_SRV_PREPARED_INPUT_COLOR                   6
#define FSR2_BIND_SRV_LUMA_HISTORY                           7
#define FSR2_BIND_SRV_LANCZOS_LUT                            8
#define FSR2_BIND_SRV_UPSCALE_MAXIMUM_BIAS_LUT               9
#define FSR2_BIND_SRV_DILATED_REACTIVE_MASKS                10
#define FSR2_BIND_SRV_EXPOSURE_MIPS                         11
#define FSR2_BIND_UAV_INTERNAL_UPSCALED                      0
#define FSR2_BIND_UAV_LOCK_STATUS                            1
#define FSR2_BIND_UAV_UPSCALED_OUTPUT                        2

#define FSR2_BIND_CB_FSR2                                    0

#include "ffx_fsr2_callbacks_hlsl.h"
#include "ffx_fsr2_common.h"
#include "ffx_fsr2_sample.h"
#include "ffx_fsr2_upsample.h"
#include "ffx_fsr2_postprocess_lock_status.h"
#include "ffx_fsr2_reproject.h"
#include "ffx_fsr2_accumulate.h"

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

#include "globals.hlsli"

FFX_FSR2_NUM_THREADS
FFX_FSR2_EMBED_ROOTSIG_CONTENT
void main(uint2 uGroupId : SV_GroupID, uint2 uGroupThreadId : SV_GroupThreadID)
{
    const uint GroupRows = (uint(DisplaySize().y) + FFX_FSR2_THREAD_GROUP_HEIGHT - 1) / FFX_FSR2_THREAD_GROUP_HEIGHT;
    uGroupId.y = GroupRows - uGroupId.y - 1;

    uint2 uDispatchThreadId = uGroupId * uint2(FFX_FSR2_THREAD_GROUP_WIDTH, FFX_FSR2_THREAD_GROUP_HEIGHT) + uGroupThreadId;

    Accumulate(uDispatchThreadId);
}

#pragma dxc diagnostic pop
