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

#include "ffx_fsr2_resources.h"

#if defined(FFX_GPU)
#ifdef __hlsl_dx_compiler
#pragma dxc diagnostic push
#pragma dxc diagnostic ignored "-Wambig-lit-shift"
#endif //__hlsl_dx_compiler
#include "ffx_core.h"
#ifdef __hlsl_dx_compiler
#pragma dxc diagnostic pop
#endif //__hlsl_dx_compiler
#endif // #if defined(FFX_GPU)

#if defined(FFX_GPU)
#ifndef FFX_FSR2_PREFER_WAVE64
#define FFX_FSR2_PREFER_WAVE64
#endif // #if defined(FFX_GPU)

#if defined(FFX_GPU)
#pragma warning(disable: 3205)  // conversion from larger type to smaller
#endif // #if defined(FFX_GPU)

#define DECLARE_SRV_REGISTER(regIndex)  t##regIndex
#define DECLARE_UAV_REGISTER(regIndex)  u##regIndex
#define DECLARE_CB_REGISTER(regIndex)   b##regIndex
#define FFX_FSR2_DECLARE_SRV(regIndex)  register(DECLARE_SRV_REGISTER(regIndex))
#define FFX_FSR2_DECLARE_UAV(regIndex)  register(DECLARE_UAV_REGISTER(regIndex))
#define FFX_FSR2_DECLARE_CB(regIndex)   register(DECLARE_CB_REGISTER(regIndex))

#if defined(FSR2_BIND_CB_FSR2)
    cbuffer cbFSR2 : FFX_FSR2_DECLARE_CB(FSR2_BIND_CB_FSR2)
    {
        FfxInt32x2    uRenderSize;
        FfxInt32x2    uDisplaySize;
        FfxInt32x2    uLumaMipDimensions;
        FfxInt32      uLumaMipLevelToUse;
        FfxUInt32     uFrameIndex;
        FfxFloat32x2  fDisplaySizeRcp;
        FfxFloat32x2  fJitter;
        FfxFloat32x4  fDeviceToViewDepth;
        FfxFloat32x2  depthclip_uv_scale;
        FfxFloat32x2  postprocessed_lockstatus_uv_scale;
        FfxFloat32x2  reactive_mask_dim_rcp;
        FfxFloat32x2  MotionVectorScale;
        FfxFloat32x2  fDownscaleFactor;
        FfxFloat32    fPreExposure;
        FfxFloat32    fTanHalfFOV;
        FfxFloat32x2  fMotionVectorJitterCancellation;
        FfxFloat32    fJitterSequenceLength;
        FfxFloat32    fLockInitialLifetime;
        FfxFloat32    fLockTickDelta;
        FfxFloat32    fDeltaTime;
        FfxFloat32    fDynamicResChangeFactor;
        FfxFloat32    fLumaMipRcp;
#define FFX_FSR2_CONSTANT_BUFFER_1_SIZE 36  // Number of 32-bit values. This must be kept in sync with the cbFSR2 size.
    };
#else
    #define iRenderSize                         0
    #define iDisplaySize                        0
    #define iLumaMipDimensions                  0
    #define iLumaMipLevelToUse                  0
    #define iFrameIndex                         0
    #define fDisplaySizeRcp                     0
    #define fJitter                             0
    #define fDeviceToViewDepth                  FfxFloat32x4(0,0,0,0)
    #define depthclip_uv_scale                  0
    #define postprocessed_lockstatus_uv_scale   0
    #define reactive_mask_dim_rcp               0
    #define MotionVectorScale                   0
    #define fDownscaleFactor                    0
    #define fPreExposure                        0
    #define fTanHalfFOV                         0
    #define fMotionVectorJitterCancellation     0
    #define fJitterSequenceLength               0
    #define fLockInitialLifetime                0
    #define fLockTickDelta                      0
    #define fDeltaTime                          0
    #define fDynamicResChangeFactor             0
    #define fLumaMipRcp                         0
#endif

#if defined(FFX_GPU)
#define FFX_FSR2_ROOTSIG_STRINGIFY(p) FFX_FSR2_ROOTSIG_STR(p)
#define FFX_FSR2_ROOTSIG_STR(p) #p
#define FFX_FSR2_ROOTSIG [RootSignature( "DescriptorTable(UAV(u0, numDescriptors = " FFX_FSR2_ROOTSIG_STRINGIFY(FFX_FSR2_RESOURCE_IDENTIFIER_COUNT) ")), " \
                                    "DescriptorTable(SRV(t0, numDescriptors = " FFX_FSR2_ROOTSIG_STRINGIFY(FFX_FSR2_RESOURCE_IDENTIFIER_COUNT) ")), " \
                                    "RootConstants(num32BitConstants=" FFX_FSR2_ROOTSIG_STRINGIFY(FFX_FSR2_CONSTANT_BUFFER_1_SIZE) ", b0), " \
                                    "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT, " \
                                                      "addressU = TEXTURE_ADDRESS_CLAMP, " \
                                                      "addressV = TEXTURE_ADDRESS_CLAMP, " \
                                                      "addressW = TEXTURE_ADDRESS_CLAMP, " \
                                                      "comparisonFunc = COMPARISON_NEVER, " \
                                                      "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK), " \
                                    "StaticSampler(s1, filter = FILTER_MIN_MAG_MIP_LINEAR, " \
                                                      "addressU = TEXTURE_ADDRESS_CLAMP, " \
                                                      "addressV = TEXTURE_ADDRESS_CLAMP, " \
                                                      "addressW = TEXTURE_ADDRESS_CLAMP, " \
                                                      "comparisonFunc = COMPARISON_NEVER, " \
                                                      "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK)" )]

#define FFX_FSR2_CONSTANT_BUFFER_2_SIZE 6  // Number of 32-bit values. This must be kept in sync with max( cbRCAS , cbSPD) size.

#define FFX_FSR2_CB2_ROOTSIG [RootSignature( "DescriptorTable(UAV(u0, numDescriptors = " FFX_FSR2_ROOTSIG_STRINGIFY(FFX_FSR2_RESOURCE_IDENTIFIER_COUNT) ")), " \
                                    "DescriptorTable(SRV(t0, numDescriptors = " FFX_FSR2_ROOTSIG_STRINGIFY(FFX_FSR2_RESOURCE_IDENTIFIER_COUNT) ")), " \
                                    "RootConstants(num32BitConstants=" FFX_FSR2_ROOTSIG_STRINGIFY(FFX_FSR2_CONSTANT_BUFFER_1_SIZE) ", b0), " \
                                    "RootConstants(num32BitConstants=" FFX_FSR2_ROOTSIG_STRINGIFY(FFX_FSR2_CONSTANT_BUFFER_2_SIZE) ", b1), " \
                                    "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT, " \
                                                      "addressU = TEXTURE_ADDRESS_CLAMP, " \
                                                      "addressV = TEXTURE_ADDRESS_CLAMP, " \
                                                      "addressW = TEXTURE_ADDRESS_CLAMP, " \
                                                      "comparisonFunc = COMPARISON_NEVER, " \
                                                      "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK), " \
                                    "StaticSampler(s1, filter = FILTER_MIN_MAG_MIP_LINEAR, " \
                                                      "addressU = TEXTURE_ADDRESS_CLAMP, " \
                                                      "addressV = TEXTURE_ADDRESS_CLAMP, " \
                                                      "addressW = TEXTURE_ADDRESS_CLAMP, " \
                                                      "comparisonFunc = COMPARISON_NEVER, " \
                                                      "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK)" )]
#if defined(FFX_FSR2_EMBED_ROOTSIG)
#define FFX_FSR2_EMBED_ROOTSIG_CONTENT FFX_FSR2_ROOTSIG
#define FFX_FSR2_EMBED_CB2_ROOTSIG_CONTENT FFX_FSR2_CB2_ROOTSIG
#else
#define FFX_FSR2_EMBED_ROOTSIG_CONTENT
#define FFX_FSR2_EMBED_CB2_ROOTSIG_CONTENT
#endif // #if FFX_FSR2_EMBED_ROOTSIG
#endif // #if defined(FFX_GPU)


FfxFloat32 LumaMipRcp()
{
    return fLumaMipRcp;
}

FfxInt32x2 LumaMipDimensions()
{
    return uLumaMipDimensions;
}

FfxInt32  LumaMipLevelToUse()
{
    return uLumaMipLevelToUse;
}

FfxFloat32x2 DownscaleFactor()
{
    return fDownscaleFactor;
}

FfxFloat32x2 Jitter()
{
    return fJitter;
}

FfxFloat32x2 MotionVectorJitterCancellation()
{
    return fMotionVectorJitterCancellation;
}

FfxInt32x2 RenderSize()
{
    return uRenderSize;
}

FfxInt32x2 DisplaySize()
{
    return uDisplaySize;
}

FfxFloat32x2 DisplaySizeRcp()
{
    return fDisplaySizeRcp;
}

FfxFloat32 JitterSequenceLength()
{
    return fJitterSequenceLength;
}

FfxFloat32 LockInitialLifetime()
{
    return fLockInitialLifetime;
}

FfxFloat32 LockTickDelta()
{
    return fLockTickDelta;
}

FfxFloat32 DeltaTime()
{
    return fDeltaTime;
}

FfxFloat32 MaxAccumulationWeight()
{
    const FfxFloat32 averageLanczosWeightPerFrame = 0.74f; // Average lanczos weight for jitter accumulated samples

    return 12; //32.0f * averageLanczosWeightPerFrame;
}

FfxFloat32 DynamicResChangeFactor()
{
    return fDynamicResChangeFactor;
}

FfxUInt32 FrameIndex()
{
    return uFrameIndex;
}

SamplerState s_PointClamp : register(s0);
SamplerState s_LinearClamp : register(s1);

// SRVs
#if defined(FFX_INTERNAL)
    Texture2D<FfxFloat32x4>                       r_input_color_jittered                    : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_INPUT_COLOR);
    Texture2D<FfxFloat32x4>                       r_motion_vectors                          : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_INPUT_MOTION_VECTORS);
    Texture2D<FfxFloat32>                         r_depth                                   : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_INPUT_DEPTH);
    Texture2D<FfxFloat32x2>                       r_exposure                                : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_INPUT_EXPOSURE);
    Texture2D<FfxFloat32>                         r_reactive_mask                           : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_INPUT_REACTIVE_MASK);
    Texture2D<FfxFloat32>                         r_transparency_and_composition_mask       : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_INPUT_TRANSPARENCY_AND_COMPOSITION_MASK);
    Texture2D<FfxUInt32>                          r_reconstructed_previous_nearest_depth    : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_RECONSTRUCTED_PREVIOUS_NEAREST_DEPTH);
    Texture2D<FfxFloat32x2>                       r_dilated_motion_vectors                  : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS);
    Texture2D<FfxFloat32>                         r_dilatedDepth                            : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_DILATED_DEPTH);
    Texture2D<FfxFloat32x4>                       r_internal_upscaled_color                 : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR);
    Texture2D<FfxFloat32x3>                       r_lock_status                             : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_LOCK_STATUS);
    Texture2D<FfxFloat32>                         r_depth_clip                              : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_DEPTH_CLIP);
    Texture2D<unorm FfxFloat32x4>                 r_prepared_input_color                    : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_PREPARED_INPUT_COLOR);
    Texture2D<unorm FfxFloat32x4>                 r_luma_history                            : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_LUMA_HISTORY);
    Texture2D<FfxFloat32x4>                       r_rcas_input                              : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_RCAS_INPUT);
    Texture2D<FfxFloat32>                         r_lanczos_lut                             : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_LANCZOS_LUT);
    Texture2D<FfxFloat32>                         r_imgMips                                 : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_AUTO_EXPOSURE);
    Texture2D<FfxFloat32>                         r_upsample_maximum_bias_lut               : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTITIER_UPSAMPLE_MAXIMUM_BIAS_LUT);
    Texture2D<FfxFloat32x2>                       r_dilated_reactive_masks                  : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_DILATED_REACTIVE_MASKS);
    Texture2D<FfxFloat32x4>                       r_debug_out                               : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_DEBUG_OUTPUT);

    // declarations not current form, no accessor functions
    Texture2D<FfxFloat32x4>                       r_transparency_mask                     : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_TRANSPARENCY_MASK);
    Texture2D<FfxFloat32x4>                       r_bias_current_color_mask               : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_BIAS_CURRENT_COLOR_MASK);
    Texture2D<FfxFloat32x4>                       r_gbuffer_albedo                        : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_GBUFFER_ALBEDO);
    Texture2D<FfxFloat32x4>                       r_gbuffer_roughness                     : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_GBUFFER_ROUGHNESS);
    Texture2D<FfxFloat32x4>                       r_gbuffer_metallic                      : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_GBUFFER_METALLIC);
    Texture2D<FfxFloat32x4>                       r_gbuffer_specular                      : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_GBUFFER_SPECULAR);
    Texture2D<FfxFloat32x4>                       r_gbuffer_subsurface                    : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_GBUFFER_SUBSURFACE);
    Texture2D<FfxFloat32x4>                       r_gbuffer_normals                       : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_GBUFFER_NORMALS);
    Texture2D<FfxFloat32x4>                       r_gbuffer_shading_mode_id               : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_GBUFFER_SHADING_MODE_ID);
    Texture2D<FfxFloat32x4>                       r_gbuffer_material_id                   : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_GBUFFER_MATERIAL_ID);
    Texture2D<FfxFloat32x4>                       r_motion_vectors_3d                     : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_VELOCITY_3D);
    Texture2D<FfxFloat32x4>                       r_is_particle_mask                      : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_IS_PARTICLE_MASK);
    Texture2D<FfxFloat32x4>                       r_animated_texture_mask                 : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_ANIMATED_TEXTURE_MASK);
    Texture2D<FfxFloat32>                         r_depth_high_res                        : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_DEPTH_HIGH_RES);
    Texture2D<FfxFloat32x4>                       r_position_view_space                   : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_POSITION_VIEW_SPACE);
    Texture2D<FfxFloat32x4>                       r_ray_tracing_hit_distance              : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_RAY_TRACING_HIT_DISTANCE);
    Texture2D<FfxFloat32x4>                       r_motion_vectors_reflection             : FFX_FSR2_DECLARE_SRV(FFX_FSR2_RESOURCE_IDENTIFIER_VELOCITY_REFLECTION);

    // UAV declarations
    RWTexture2D<FfxUInt32>                        rw_reconstructed_previous_nearest_depth   : FFX_FSR2_DECLARE_UAV(FFX_FSR2_RESOURCE_IDENTIFIER_RECONSTRUCTED_PREVIOUS_NEAREST_DEPTH);
    RWTexture2D<FfxFloat32x2>                     rw_dilated_motion_vectors                 : FFX_FSR2_DECLARE_UAV(FFX_FSR2_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS);
    RWTexture2D<FfxFloat32>                       rw_dilatedDepth                           : FFX_FSR2_DECLARE_UAV(FFX_FSR2_RESOURCE_IDENTIFIER_DILATED_DEPTH);
    RWTexture2D<FfxFloat32x4>                     rw_internal_upscaled_color                : FFX_FSR2_DECLARE_UAV(FFX_FSR2_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR);
    RWTexture2D<FfxFloat32x3>                     rw_lock_status                            : FFX_FSR2_DECLARE_UAV(FFX_FSR2_RESOURCE_IDENTIFIER_LOCK_STATUS);
    RWTexture2D<FfxFloat32>                       rw_depth_clip                             : FFX_FSR2_DECLARE_UAV(FFX_FSR2_RESOURCE_IDENTIFIER_DEPTH_CLIP);
    RWTexture2D<unorm FfxFloat32x4>               rw_prepared_input_color                   : FFX_FSR2_DECLARE_UAV(FFX_FSR2_RESOURCE_IDENTIFIER_PREPARED_INPUT_COLOR);
    RWTexture2D<unorm FfxFloat32x4>               rw_luma_history                           : FFX_FSR2_DECLARE_UAV(FFX_FSR2_RESOURCE_IDENTIFIER_LUMA_HISTORY);
    RWTexture2D<FfxFloat32x4>                     rw_upscaled_output                        : FFX_FSR2_DECLARE_UAV(FFX_FSR2_RESOURCE_IDENTIFIER_UPSCALED_OUTPUT);

    globallycoherent RWTexture2D<FfxFloat32>      rw_img_mip_shading_change               : FFX_FSR2_DECLARE_UAV(FFX_FSR2_RESOURCE_IDENTIFIER_AUTO_EXPOSURE_MIPMAP_SHADING_CHANGE);
    globallycoherent RWTexture2D<FfxFloat32>      rw_img_mip_5                            : FFX_FSR2_DECLARE_UAV(FFX_FSR2_RESOURCE_IDENTIFIER_AUTO_EXPOSURE_MIPMAP_5);
    RWTexture2D<unorm FfxFloat32x2>               rw_dilated_reactive_masks               : FFX_FSR2_DECLARE_UAV(FFX_FSR2_RESOURCE_IDENTIFIER_DILATED_REACTIVE_MASKS);
    RWTexture2D<FfxFloat32x2>                     rw_exposure                             : FFX_FSR2_DECLARE_UAV(FFX_FSR2_RESOURCE_IDENTIFIER_EXPOSURE);
    globallycoherent RWTexture2D<FfxUInt32>       rw_spd_global_atomic                    : FFX_FSR2_DECLARE_UAV(FFX_FSR2_RESOURCE_IDENTIFIER_SPD_ATOMIC_COUNT);
    RWTexture2D<FfxFloat32x4>                     rw_debug_out                            : FFX_FSR2_DECLARE_UAV(FFX_FSR2_RESOURCE_IDENTIFIER_DEBUG_OUTPUT);
    
#else // #if defined(FFX_INTERNAL)
    #if defined FSR2_BIND_SRV_INPUT_COLOR
        Texture2D<FfxFloat32x4>                   r_input_color_jittered                  : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_INPUT_COLOR);
    #endif
    #if defined FSR2_BIND_SRV_MOTION_VECTORS
        Texture2D<FfxFloat32x4>                   r_motion_vectors                        : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_MOTION_VECTORS);
    #endif
    #if defined FSR2_BIND_SRV_DEPTH
        Texture2D<FfxFloat32>                     r_depth                                 : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_DEPTH);
    #endif 
    #if defined FSR2_BIND_SRV_EXPOSURE
        Texture2D<FfxFloat32x2>                   r_exposure                              : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_EXPOSURE);
    #endif
    #if defined FSR2_BIND_SRV_REACTIVE_MASK
        Texture2D<FfxFloat32>                     r_reactive_mask                         : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_REACTIVE_MASK);
    #endif 
    #if defined FSR2_BIND_SRV_TRANSPARENCY_AND_COMPOSITION_MASK
        Texture2D<FfxFloat32>                     r_transparency_and_composition_mask     : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_TRANSPARENCY_AND_COMPOSITION_MASK);
    #endif
    #if defined FSR2_BIND_SRV_RECONSTRUCTED_PREV_NEAREST_DEPTH
        Texture2D<FfxUInt32>                      r_reconstructed_previous_nearest_depth  : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_RECONSTRUCTED_PREV_NEAREST_DEPTH);
    #endif 
    #if defined FSR2_BIND_SRV_DILATED_MOTION_VECTORS
       Texture2D<FfxFloat32x2>                    r_dilated_motion_vectors                : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_DILATED_MOTION_VECTORS);
    #endif
    #if defined FSR2_BIND_SRV_DILATED_DEPTH
        Texture2D<FfxFloat32>                     r_dilatedDepth                          : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_DILATED_DEPTH);
    #endif
    #if defined FSR2_BIND_SRV_INTERNAL_UPSCALED
        Texture2D<FfxFloat32x4>                   r_internal_upscaled_color               : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_INTERNAL_UPSCALED);
    #endif
    #if defined FSR2_BIND_SRV_LOCK_STATUS
        Texture2D<FfxFloat32x3>                   r_lock_status                           : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_LOCK_STATUS);
    #endif
    #if defined FSR2_BIND_SRV_DEPTH_CLIP
        Texture2D<FfxFloat32>                     r_depth_clip                            : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_DEPTH_CLIP);
    #endif
    #if defined FSR2_BIND_SRV_PREPARED_INPUT_COLOR
        Texture2D<unorm FfxFloat32x4>             r_prepared_input_color                    : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_PREPARED_INPUT_COLOR);
    #endif
    #if defined FSR2_BIND_SRV_LUMA_HISTORY
        Texture2D<unorm FfxFloat32x4>             r_luma_history                          : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_LUMA_HISTORY);
    #endif
    #if defined FSR2_BIND_SRV_RCAS_INPUT
        Texture2D<FfxFloat32x4>                   r_rcas_input                            : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_RCAS_INPUT);
    #endif
    #if defined FSR2_BIND_SRV_LANCZOS_LUT
        Texture2D<FfxFloat32>                     r_lanczos_lut                           : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_LANCZOS_LUT);
    #endif
    #if defined FSR2_BIND_SRV_EXPOSURE_MIPS
        Texture2D<FfxFloat32>                     r_imgMips                               : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_EXPOSURE_MIPS);
    #endif
    #if defined FSR2_BIND_SRV_UPSCALE_MAXIMUM_BIAS_LUT
        Texture2D<FfxFloat32>                     r_upsample_maximum_bias_lut             : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_UPSCALE_MAXIMUM_BIAS_LUT);
    #endif
    #if defined FSR2_BIND_SRV_DILATED_REACTIVE_MASKS
        Texture2D<FfxFloat32x2>                   r_dilated_reactive_masks                : FFX_FSR2_DECLARE_SRV(FSR2_BIND_SRV_DILATED_REACTIVE_MASKS);
    #endif

    // UAV declarations
    #if defined FSR2_BIND_UAV_RECONSTRUCTED_PREV_NEAREST_DEPTH
        RWTexture2D<FfxUInt32>                    rw_reconstructed_previous_nearest_depth : FFX_FSR2_DECLARE_UAV(FSR2_BIND_UAV_RECONSTRUCTED_PREV_NEAREST_DEPTH);
    #endif
    #if defined FSR2_BIND_UAV_DILATED_MOTION_VECTORS
        RWTexture2D<FfxFloat32x2>                 rw_dilated_motion_vectors               : FFX_FSR2_DECLARE_UAV(FSR2_BIND_UAV_DILATED_MOTION_VECTORS);
    #endif
    #if defined FSR2_BIND_UAV_DILATED_DEPTH
        RWTexture2D<FfxFloat32>                   rw_dilatedDepth                         : FFX_FSR2_DECLARE_UAV(FSR2_BIND_UAV_DILATED_DEPTH);
    #endif
    #if defined FSR2_BIND_UAV_INTERNAL_UPSCALED
        RWTexture2D<FfxFloat32x4>                 rw_internal_upscaled_color              : FFX_FSR2_DECLARE_UAV(FSR2_BIND_UAV_INTERNAL_UPSCALED);
    #endif
    #if defined FSR2_BIND_UAV_LOCK_STATUS
        RWTexture2D<FfxFloat32x3>                 rw_lock_status                          : FFX_FSR2_DECLARE_UAV(FSR2_BIND_UAV_LOCK_STATUS);
    #endif
    #if defined FSR2_BIND_UAV_DEPTH_CLIP
        RWTexture2D<FfxFloat32>                   rw_depth_clip                           : FFX_FSR2_DECLARE_UAV(FSR2_BIND_UAV_DEPTH_CLIP);
    #endif
    #if defined FSR2_BIND_UAV_PREPARED_INPUT_COLOR
        RWTexture2D<unorm FfxFloat32x4>           rw_prepared_input_color                   : FFX_FSR2_DECLARE_UAV(FSR2_BIND_UAV_PREPARED_INPUT_COLOR);
    #endif
    #if defined FSR2_BIND_UAV_LUMA_HISTORY
        RWTexture2D<unorm FfxFloat32x4>           rw_luma_history                         : FFX_FSR2_DECLARE_UAV(FSR2_BIND_UAV_LUMA_HISTORY);
    #endif
    #if defined FSR2_BIND_UAV_UPSCALED_OUTPUT
        RWTexture2D<FfxFloat32x4>                 rw_upscaled_output                      : FFX_FSR2_DECLARE_UAV(FSR2_BIND_UAV_UPSCALED_OUTPUT);
    #endif
    #if defined FSR2_BIND_UAV_EXPOSURE_MIP_LUMA_CHANGE
        globallycoherent RWTexture2D<FfxFloat32>  rw_img_mip_shading_change               : FFX_FSR2_DECLARE_UAV(FSR2_BIND_UAV_EXPOSURE_MIP_LUMA_CHANGE);
    #endif
    #if defined FSR2_BIND_UAV_EXPOSURE_MIP_5
        globallycoherent RWTexture2D<FfxFloat32>  rw_img_mip_5                            : FFX_FSR2_DECLARE_UAV(FSR2_BIND_UAV_EXPOSURE_MIP_5);
    #endif
    #if defined FSR2_BIND_UAV_DILATED_REACTIVE_MASKS
        RWTexture2D<unorm FfxFloat32x2>           rw_dilated_reactive_masks               : FFX_FSR2_DECLARE_UAV(FSR2_BIND_UAV_DILATED_REACTIVE_MASKS);
    #endif
    #if defined FSR2_BIND_UAV_EXPOSURE
        RWTexture2D<FfxFloat32x2>                 rw_exposure                             : FFX_FSR2_DECLARE_UAV(FSR2_BIND_UAV_EXPOSURE);
    #endif
    #if defined FSR2_BIND_UAV_SPD_GLOBAL_ATOMIC
        globallycoherent RWTexture2D<FfxUInt32>   rw_spd_global_atomic                    : FFX_FSR2_DECLARE_UAV(FSR2_BIND_UAV_SPD_GLOBAL_ATOMIC);
    #endif
#endif // #if defined(FFX_INTERNAL)

FfxFloat32 LoadMipLuma(FfxUInt32x2 iPxPos, FfxUInt32 mipLevel)
{
#if defined(FSR2_BIND_SRV_EXPOSURE_MIPS) || defined(FFX_INTERNAL)
    return r_imgMips.mips[mipLevel][iPxPos];
#else
    return 0.f;
#endif
}

FfxFloat32 SampleMipLuma(FfxFloat32x2 fUV, FfxUInt32 mipLevel)
{
#if defined(FSR2_BIND_SRV_EXPOSURE_MIPS) || defined(FFX_INTERNAL)
    fUV *= depthclip_uv_scale;
    return r_imgMips.SampleLevel(s_LinearClamp, fUV, mipLevel);
#else
    return 0.f;
#endif

}

//
// a 0 0 0   x
// 0 b 0 0   y
// 0 0 c d   z
// 0 0 e 0   1
// 
// z' = (z*c+d)/(z*e)
// z' = (c/e) + d/(z*e)
// z' - (c/e) = d/(z*e)
// (z'e - c)/e = d/(z*e)
// e / (z'e - c) = (z*e)/d
// (e * d) / (z'e - c) = z*e
// z = d / (z'e - c)
FfxFloat32 ConvertFromDeviceDepthToViewSpace(FfxFloat32 fDeviceDepth)
{
    return -fDeviceToViewDepth[2] / (fDeviceDepth * fDeviceToViewDepth[1] - fDeviceToViewDepth[0]);
}

FfxFloat32 LoadInputDepth(FfxUInt32x2 iPxPos)
{
#if defined(FSR2_BIND_SRV_DEPTH) || defined(FFX_INTERNAL)
    return r_depth[iPxPos];
#else
    return 0.f;
#endif
}

FfxFloat32 LoadReactiveMask(FfxUInt32x2 iPxPos)
{
#if defined(FSR2_BIND_SRV_REACTIVE_MASK) || defined(FFX_INTERNAL)
    return r_reactive_mask[iPxPos];
#else 
    return 0.f;
#endif
}

FfxFloat32x4 GatherReactiveMask(FfxUInt32x2 iPxPos)
{
#if defined(FSR2_BIND_SRV_REACTIVE_MASK) || defined(FFX_INTERNAL)
    return r_reactive_mask.GatherRed(s_LinearClamp, FfxFloat32x2(iPxPos) * reactive_mask_dim_rcp);
#else
    return 0.f;
#endif
}

FfxFloat32 LoadTransparencyAndCompositionMask(FfxUInt32x2 iPxPos)
{
#if defined(FSR2_BIND_SRV_TRANSPARENCY_AND_COMPOSITION_MASK) || defined(FFX_INTERNAL)
    return r_transparency_and_composition_mask[iPxPos];
#else
    return 0.f;
#endif
}

FfxFloat32 SampleTransparencyAndCompositionMask(FfxFloat32x2 fUV)
{
#if defined(FSR2_BIND_SRV_TRANSPARENCY_AND_COMPOSITION_MASK) || defined(FFX_INTERNAL)
    fUV *= depthclip_uv_scale;
    return r_transparency_and_composition_mask.SampleLevel(s_LinearClamp, fUV, 0);
#else
    return 0.f;
#endif
}

FfxFloat32 PreExposure()
{
    return fPreExposure;
}

FfxFloat32x3 LoadInputColor(FfxUInt32x2 iPxPos)
{
#if defined(FSR2_BIND_SRV_INPUT_COLOR) || defined(FFX_INTERNAL)
    return r_input_color_jittered[iPxPos].rgb / PreExposure();
#else
    return 0;
#endif
}

FfxFloat32x3 LoadInputColorWithoutPreExposure(FfxUInt32x2 iPxPos)
{
#if defined(FSR2_BIND_SRV_INPUT_COLOR) || defined(FFX_INTERNAL)
    return r_input_color_jittered[iPxPos].rgb;
#else
    return 0;
#endif
}

FfxFloat32x3 LoadPreparedInputColor(FfxUInt32x2 iPxPos)
{
#if defined(FSR2_BIND_SRV_PREPARED_INPUT_COLOR) || defined(FFX_INTERNAL)
    return r_prepared_input_color[iPxPos].rgb;
#else
    return 0.f;
#endif
}

FfxFloat32 LoadPreparedInputColorLuma(FfxUInt32x2 iPxPos)
{
#if defined(FSR2_BIND_SRV_PREPARED_INPUT_COLOR) || defined(FFX_INTERNAL)
    return r_prepared_input_color[iPxPos].a;
#else
    return 0.f;
#endif
}

FfxFloat32x2 LoadInputMotionVector(FfxUInt32x2 iPxDilatedMotionVectorPos)
{
#if defined(FSR2_BIND_SRV_MOTION_VECTORS) || defined(FFX_INTERNAL)
    FfxFloat32x2 fSrcMotionVector = r_motion_vectors[iPxDilatedMotionVectorPos].xy;
#else
    FfxFloat32x2 fSrcMotionVector = 0.f;
#endif

    FfxFloat32x2 fUvMotionVector = fSrcMotionVector * MotionVectorScale;

#if FFX_FSR2_OPTION_JITTERED_MOTION_VECTORS
    fUvMotionVector -= fMotionVectorJitterCancellation;
#endif

    return fUvMotionVector;
}

FfxFloat32x4 LoadHistory(FfxUInt32x2 iPxHistory)
{
#if defined(FSR2_BIND_SRV_INTERNAL_UPSCALED) || defined(FFX_INTERNAL)
    return r_internal_upscaled_color[iPxHistory];
#else
    return 0.f;
#endif
}

FfxFloat32x4 LoadRwInternalUpscaledColorAndWeight(FfxUInt32x2 iPxPos)
{
#if defined(FSR2_BIND_UAV_INTERNAL_UPSCALED) || defined(FFX_INTERNAL)
    return rw_internal_upscaled_color[iPxPos];
#else
    return 0.f;
#endif
}

void StoreLumaHistory(FfxUInt32x2 iPxPos, FfxFloat32x4 fLumaHistory)
{
#if defined(FSR2_BIND_UAV_LUMA_HISTORY) || defined(FFX_INTERNAL)
    rw_luma_history[iPxPos] = fLumaHistory;
#endif
}

FfxFloat32x4 LoadRwLumaHistory(FfxUInt32x2 iPxPos)
{
#if defined(FSR2_BIND_UAV_LUMA_HISTORY) || defined(FFX_INTERNAL)
    return rw_luma_history[iPxPos];
#else
    return 1.f;
#endif
}

FfxFloat32 LoadLumaStabilityFactor(FfxUInt32x2 iPxPos)
{
#if defined(FSR2_BIND_SRV_LUMA_HISTORY) || defined(FFX_INTERNAL)
    return r_luma_history[iPxPos].w;
#else
    return 0.f;
#endif
}

FfxFloat32 SampleLumaStabilityFactor(FfxFloat32x2 fUV)
{
#if defined(FSR2_BIND_SRV_LUMA_HISTORY) || defined(FFX_INTERNAL)
    fUV *= depthclip_uv_scale;
    return r_luma_history.SampleLevel(s_LinearClamp, fUV, 0).w;
#else
    return 0.f;
#endif
}

void StoreReprojectedHistory(FfxUInt32x2 iPxHistory, FfxFloat32x4 fHistory)
{
#if defined(FSR2_BIND_UAV_INTERNAL_UPSCALED) || defined(FFX_INTERNAL)
    rw_internal_upscaled_color[iPxHistory] = fHistory;
#endif
}

void StoreInternalColorAndWeight(FfxUInt32x2 iPxPos, FfxFloat32x4 fColorAndWeight)
{
#if defined(FSR2_BIND_UAV_INTERNAL_UPSCALED) || defined(FFX_INTERNAL)
    rw_internal_upscaled_color[iPxPos] = fColorAndWeight;
#endif
}

void StoreUpscaledOutput(FfxUInt32x2 iPxPos, FfxFloat32x3 fColor)
{
#if defined(FSR2_BIND_UAV_UPSCALED_OUTPUT) || defined(FFX_INTERNAL)
    rw_upscaled_output[iPxPos] = FfxFloat32x4(fColor * PreExposure(), 1.f);
#endif
}

//LOCK_LIFETIME_REMAINING == 0
//Should make LockInitialLifetime() return a const 1.0f later
FfxFloat32x3 LoadLockStatus(FfxUInt32x2 iPxPos)
{
#if defined(FSR2_BIND_SRV_LOCK_STATUS) || defined(FFX_INTERNAL)
    FfxFloat32x3 fLockStatus = r_lock_status[iPxPos];

    fLockStatus[0] -= LockInitialLifetime() * 2.0f;
    return fLockStatus;
#else
    return 0.f;
#endif

    
}

FfxFloat32x3 LoadRwLockStatus(FfxUInt32x2 iPxPos)
{
#if defined(FSR2_BIND_UAV_LOCK_STATUS) || defined(FFX_INTERNAL)
    FfxFloat32x3 fLockStatus = rw_lock_status[iPxPos];

    fLockStatus[0] -= LockInitialLifetime() * 2.0f;

    return fLockStatus;
#else
    return 0.f;
#endif
}

void StoreLockStatus(FfxUInt32x2 iPxPos, FfxFloat32x3 fLockstatus)
{
#if defined(FSR2_BIND_UAV_LOCK_STATUS) || defined(FFX_INTERNAL)
    fLockstatus[0] += LockInitialLifetime() * 2.0f;

    rw_lock_status[iPxPos] = fLockstatus;
#endif
}

void StorePreparedInputColor(FFX_PARAMETER_IN FfxUInt32x2 iPxPos, FFX_PARAMETER_IN FfxFloat32x4 fTonemapped)
{
#if defined(FSR2_BIND_UAV_PREPARED_INPUT_COLOR) || defined(FFX_INTERNAL)
    rw_prepared_input_color[iPxPos] = fTonemapped;
#endif
}

FfxBoolean IsResponsivePixel(FfxUInt32x2 iPxPos)
{
    return FFX_FALSE; //not supported in prototype
}

FfxFloat32 LoadDepthClip(FfxUInt32x2 iPxPos)
{
#if defined(FSR2_BIND_SRV_DEPTH_CLIP) || defined(FFX_INTERNAL)
    return r_depth_clip[iPxPos];
#else
    return 0.f;
#endif
}

FfxFloat32 SampleDepthClip(FfxFloat32x2 fUV)
{
#if defined(FSR2_BIND_SRV_DEPTH_CLIP) || defined(FFX_INTERNAL)
    fUV *= depthclip_uv_scale;
    return r_depth_clip.SampleLevel(s_LinearClamp, fUV, 0);
#else
    return 0.f;
#endif
}

FfxFloat32x3 SampleLockStatus(FfxFloat32x2 fUV)
{
#if defined(FSR2_BIND_SRV_LOCK_STATUS) || defined(FFX_INTERNAL)
    fUV *= postprocessed_lockstatus_uv_scale;
    FfxFloat32x3 fLockStatus = r_lock_status.SampleLevel(s_LinearClamp, fUV, 0);
    fLockStatus[0] -= LockInitialLifetime() * 2.0f;
    return fLockStatus;
#else
    return 0.f;
#endif
}

void StoreDepthClip(FfxUInt32x2 iPxPos, FfxFloat32 fClip)
{
#if defined(FSR2_BIND_UAV_DEPTH_CLIP) || defined(FFX_INTERNAL)
    rw_depth_clip[iPxPos] = fClip;
#endif
}

FfxFloat32 TanHalfFoV()
{
    return fTanHalfFOV;
}

FfxFloat32 LoadSceneDepth(FfxUInt32x2 iPxInput)
{
#if defined(FSR2_BIND_SRV_DEPTH) || defined(FFX_INTERNAL)
    return r_depth[iPxInput];
#else
    return 0.f;
#endif
}

FfxFloat32 LoadReconstructedPrevDepth(FfxUInt32x2 iPxPos)
{
#if defined(FSR2_BIND_SRV_RECONSTRUCTED_PREV_NEAREST_DEPTH) || defined(FFX_INTERNAL)
    return asfloat(r_reconstructed_previous_nearest_depth[iPxPos]);
#else 
    return 0;
#endif
}

void StoreReconstructedDepth(FfxUInt32x2 iPxSample, FfxFloat32 fDepth)
{
    FfxUInt32 uDepth = asuint(fDepth);
#if defined(FSR2_BIND_UAV_RECONSTRUCTED_PREV_NEAREST_DEPTH) || defined(FFX_INTERNAL)
    #if FFX_FSR2_OPTION_INVERTED_DEPTH
        InterlockedMax(rw_reconstructed_previous_nearest_depth[iPxSample], uDepth);
    #else
        InterlockedMin(rw_reconstructed_previous_nearest_depth[iPxSample], uDepth); // min for standard, max for inverted depth
    #endif
#endif
}

void SetReconstructedDepth(FfxUInt32x2 iPxSample, const FfxUInt32 uValue)
{
#if defined(FSR2_BIND_UAV_RECONSTRUCTED_PREV_NEAREST_DEPTH) || defined(FFX_INTERNAL)
    rw_reconstructed_previous_nearest_depth[iPxSample] = uValue;
#endif
}

void StoreDilatedDepth(FFX_PARAMETER_IN FfxUInt32x2 iPxPos, FFX_PARAMETER_IN FfxFloat32 fDepth)
{
#if defined(FSR2_BIND_UAV_DILATED_DEPTH) || defined(FFX_INTERNAL)
    rw_dilatedDepth[iPxPos] = fDepth;
#endif
}

void StoreDilatedMotionVector(FFX_PARAMETER_IN FfxUInt32x2 iPxPos, FFX_PARAMETER_IN FfxFloat32x2 fMotionVector)
{
#if defined(FSR2_BIND_UAV_DILATED_MOTION_VECTORS) || defined(FFX_INTERNAL)
    rw_dilated_motion_vectors[iPxPos] = fMotionVector;
#endif
}

FfxFloat32x2 LoadDilatedMotionVector(FfxUInt32x2 iPxInput)
{
#if defined(FSR2_BIND_SRV_DILATED_MOTION_VECTORS) || defined(FFX_INTERNAL)
    return r_dilated_motion_vectors[iPxInput].xy;
#else 
    return 0.f;
#endif
}

FfxFloat32x2 SampleDilatedMotionVector(FfxFloat32x2 fUV)
{
#if defined(FSR2_BIND_SRV_DILATED_MOTION_VECTORS) || defined(FFX_INTERNAL)
    fUV *= depthclip_uv_scale; // TODO: assuming these are (RenderSize() / MaxRenderSize())
    return r_dilated_motion_vectors.SampleLevel(s_LinearClamp, fUV, 0);
#else
    return 0.f;
#endif
}

FfxFloat32 LoadDilatedDepth(FfxUInt32x2 iPxInput)
{
#if defined(FSR2_BIND_SRV_DILATED_DEPTH) || defined(FFX_INTERNAL)
    return r_dilatedDepth[iPxInput];
#else
    return 0.f;
#endif
}

FfxFloat32 Exposure()
{
    // return 1.0f;
    #if defined(FSR2_BIND_SRV_EXPOSURE) || defined(FFX_INTERNAL)
        FfxFloat32 exposure = r_exposure[FfxUInt32x2(0, 0)].x;
    #else
        FfxFloat32 exposure = 1.f;
    #endif

    if (exposure == 0.0f) {
        exposure = 1.0f;
    }

    return exposure;
}

FfxFloat32 SampleLanczos2Weight(FfxFloat32 x)
{
#if defined(FSR2_BIND_SRV_LANCZOS_LUT) || defined(FFX_INTERNAL)
    return r_lanczos_lut.SampleLevel(s_LinearClamp, FfxFloat32x2(x / 2, 0.5f), 0);
#else
    return 0.f;
#endif
}

FfxFloat32 SampleUpsampleMaximumBias(FfxFloat32x2 uv)
{
#if defined(FSR2_BIND_SRV_UPSCALE_MAXIMUM_BIAS_LUT) || defined(FFX_INTERNAL)
    // Stored as a SNORM, so make sure to multiply by 2 to retrieve the actual expected range.
    return FfxFloat32(2.0) * r_upsample_maximum_bias_lut.SampleLevel(s_LinearClamp, abs(uv) * 2.0, 0);
#else
    return 0.f;
#endif
}

FfxFloat32x2 SampleDilatedReactiveMasks(FfxFloat32x2 fUV)
{
#if defined(FSR2_BIND_SRV_DILATED_REACTIVE_MASKS) || defined(FFX_INTERNAL)
    fUV *= depthclip_uv_scale;
	return r_dilated_reactive_masks.SampleLevel(s_LinearClamp, fUV, 0);
#else
	return 0.f;
#endif
}

FfxFloat32x2 LoadDilatedReactiveMasks(FFX_PARAMETER_IN FfxUInt32x2 iPxPos)
{
#if defined(FSR2_BIND_SRV_DILATED_REACTIVE_MASKS) || defined(FFX_INTERNAL)
    return r_dilated_reactive_masks[iPxPos];
#else
    return 0.f;
#endif
}

void StoreDilatedReactiveMasks(FFX_PARAMETER_IN FfxUInt32x2 iPxPos, FFX_PARAMETER_IN FfxFloat32x2 fDilatedReactiveMasks)
{
#if defined(FSR2_BIND_UAV_DILATED_REACTIVE_MASKS) || defined(FFX_INTERNAL)
    rw_dilated_reactive_masks[iPxPos] = fDilatedReactiveMasks;
#endif
}

#endif // #if defined(FFX_GPU)
