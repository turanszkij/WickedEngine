/**********************************************************************
Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
********************************************************************/

#ifndef FFX_DNSR_REFLECTIONS_RESOLVE_SPATIAL
#define FFX_DNSR_REFLECTIONS_RESOLVE_SPATIAL

#include "ffx_denoiser_reflections_common.h"

void FFX_DNSR_Reflections_LoadWithOffset(int2 dispatch_thread_id, int2 offset, out min16float3 radiance, out min16float3 normal, out float depth) {
    dispatch_thread_id += offset;
    radiance = FFX_DNSR_Reflections_LoadRadianceFP16(dispatch_thread_id);
    normal = FFX_DNSR_Reflections_LoadNormalFP16(dispatch_thread_id);
    depth = FFX_DNSR_Reflections_LoadDepth(dispatch_thread_id);
}

void FFX_DNSR_Reflections_StoreWithOffset(int2 group_thread_id, int2 offset, min16float3 radiance, min16float3 normal, float depth) {
    group_thread_id += offset;
    FFX_DNSR_Reflections_StoreInGroupSharedMemory(group_thread_id, radiance, normal, depth); // Pack ray length and radiance together
}

void FFX_DNSR_Reflections_InitializeGroupSharedMemory(int2 dispatch_thread_id, int2 group_thread_id, uint samples_per_quad, uint screenWidth) {
    // Load 16x16 region into shared memory.
    int2 offset_0 = 0;
    int2 offset_1 = int2(8, 0);
    int2 offset_2 = int2(0, 8);
    int2 offset_3 = int2(8, 8);

    min16float3 radiance_0;
    min16float3 normal_0;
    float depth_0;

    min16float3 radiance_1;
    min16float3 normal_1;
    float depth_1;

    min16float3 radiance_2;
    min16float3 normal_2;
    float depth_2;

    min16float3 radiance_3;
    min16float3 normal_3;
    float depth_3;

    /// XA
    /// BC

    dispatch_thread_id -= 4; // 1 + 3 => additional band + left band
    FFX_DNSR_Reflections_LoadWithOffset(dispatch_thread_id, offset_0, radiance_0, normal_0, depth_0); // X
    FFX_DNSR_Reflections_LoadWithOffset(dispatch_thread_id, offset_1, radiance_1, normal_1, depth_1); // A
    FFX_DNSR_Reflections_LoadWithOffset(dispatch_thread_id, offset_2, radiance_2, normal_2, depth_2); // B
    FFX_DNSR_Reflections_LoadWithOffset(dispatch_thread_id, offset_3, radiance_3, normal_3, depth_3); // C

    FFX_DNSR_Reflections_StoreWithOffset(group_thread_id, offset_0, radiance_0, normal_0, depth_0); // X
    FFX_DNSR_Reflections_StoreWithOffset(group_thread_id, offset_1, radiance_1, normal_1, depth_1); // A
    FFX_DNSR_Reflections_StoreWithOffset(group_thread_id, offset_2, radiance_2, normal_2, depth_2); // B
    FFX_DNSR_Reflections_StoreWithOffset(group_thread_id, offset_3, radiance_3, normal_3, depth_3); // C
}

min16float3 FFX_DNSR_Reflections_Resolve(int2 group_thread_id, min16float3 center_radiance, min16float3 center_normal, float depth_sigma, float center_depth) {
    float3 accumulated_radiance = center_radiance;
    float accumulated_weight = 1;

    const float normal_sigma = 64.0;

    // First 15 numbers of Halton(2,3) streteched to [-3,3]
    const int2 reuse_offsets[] = {
        0, 1,
        -2, 1,
        2, -3,
        -3, 0,
        1, 2,
        -1, -2,
        3, 0,
        -3, 3,
        0, -3,
        -1, -1,
        2, 1,
        -2, -2,
        1, 0,
        0, 2,
        3, -1
    };
    const uint sample_count = 15;

    int2 mirror = 2 * (group_thread_id & 0b1) - 1;

    for (int i = 0; i < sample_count; ++i) {
        int2 new_idx = group_thread_id + mirror * reuse_offsets[i];
        min16float3 normal = FFX_DNSR_Reflections_LoadNormalFromGroupSharedMemory(new_idx);
        float depth = FFX_DNSR_Reflections_LoadDepthFromGroupSharedMemory(new_idx);
        min16float3 radiance = FFX_DNSR_Reflections_LoadRadianceFromGroupSharedMemory(new_idx);
        float weight = 1
            * FFX_DNSR_Reflections_GetEdgeStoppingNormalWeight((float3)center_normal, (float3)normal, normal_sigma)
            * FFX_DNSR_Reflections_Gaussian(center_depth, depth, depth_sigma)
            ;

        // Accumulate all contributions.
        accumulated_weight += weight;
        accumulated_radiance += weight * radiance.xyz;
    }

    accumulated_radiance /= max(accumulated_weight, 0.00001);
    return (min16float3)accumulated_radiance;
}

void FFX_DNSR_Reflections_ResolveSpatial(int2 dispatch_thread_id, int2 group_thread_id, uint samples_per_quad, uint2 screen_size) {
    // First check if we have to denoise or if a simple copy is enough
    uint tile_meta_data_index = FFX_DNSR_Reflections_GetTileMetaDataIndex(dispatch_thread_id, screen_size.x);
    tile_meta_data_index = WaveReadLaneFirst(tile_meta_data_index);
    bool needs_denoiser = FFX_DNSR_Reflections_LoadTileMetaDataMask(tile_meta_data_index);

    [branch]
    if (needs_denoiser) {
        float center_roughness = FFX_DNSR_Reflections_LoadRoughness(dispatch_thread_id);
        FFX_DNSR_Reflections_InitializeGroupSharedMemory(dispatch_thread_id, group_thread_id, samples_per_quad, screen_size.x);
        GroupMemoryBarrierWithGroupSync();

        group_thread_id += 4; // Center threads in groupshared memory
        min16float3 center_radiance = FFX_DNSR_Reflections_LoadRadianceFromGroupSharedMemory(group_thread_id);

        if (!FFX_DNSR_Reflections_IsGlossyReflection(center_roughness) || FFX_DNSR_Reflections_IsMirrorReflection(center_roughness)) {
            FFX_DNSR_Reflections_StoreSpatiallyDenoisedReflections(dispatch_thread_id, center_radiance);
            return;
        }

        min16float3 center_normal = FFX_DNSR_Reflections_LoadNormalFromGroupSharedMemory(group_thread_id);
        float center_depth = FFX_DNSR_Reflections_LoadDepthFromGroupSharedMemory(group_thread_id);

        min16float3 resolved_radiance = FFX_DNSR_Reflections_Resolve(group_thread_id, center_radiance, center_normal, g_depth_sigma, center_depth);
        FFX_DNSR_Reflections_StoreSpatiallyDenoisedReflections(dispatch_thread_id, resolved_radiance);
    }
    else {
        min16float3 radiance = FFX_DNSR_Reflections_LoadRadianceFP16(dispatch_thread_id);
        FFX_DNSR_Reflections_StoreSpatiallyDenoisedReflections(dispatch_thread_id, radiance);
    }
}

#endif //FFX_DNSR_REFLECTIONS_RESOLVE_SPATIAL
