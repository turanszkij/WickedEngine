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

#ifndef FFX_DNSR_REFLECTIONS_BLUR
#define FFX_DNSR_REFLECTIONS_BLUR

#include "ffx_denoiser_reflections_common.h"

min16float FFX_DNSR_Reflections_GaussianWeight(int x, int y) {
    uint weights[] = { 6, 4, 1 };
    return min16float(weights[abs(x)] * weights[abs(y)]) / 256.0;
}

min16float3 FFX_DNSR_Reflections_Resolve(int2 group_thread_id, min16float center_roughness, min16float roughness_sigma_min, min16float roughness_sigma_max) {
    min16float3 sum = 0.0;
    min16float total_weight = 0.0;

    const int radius = 2;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int2 texel_coords = group_thread_id + int2(dx, dy);

            min16float3 radiance;
            min16float roughness;
            FFX_DNSR_Reflections_LoadFromGroupSharedMemory(texel_coords, radiance, roughness);

            min16float weight = 1
                * FFX_DNSR_Reflections_GaussianWeight(dx, dy)
                * FFX_DNSR_Reflections_GetEdgeStoppingRoughnessWeightFP16(center_roughness, roughness, roughness_sigma_min, roughness_sigma_max);
            sum += weight * radiance;
            total_weight += weight;
        }
    }

    sum /= max(total_weight, 0.0001);
    return min16float3(sum);
}

void FFX_DNSR_Reflections_LoadWithOffset(int2 dispatch_thread_id, int2 offset, out min16float3 radiance, out min16float roughness) {
    dispatch_thread_id += offset;
    radiance = FFX_DNSR_Reflections_LoadRadianceFP16(dispatch_thread_id);
    roughness = FFX_DNSR_Reflections_LoadRoughnessFP16(dispatch_thread_id);
}

void FFX_DNSR_Reflections_StoreWithOffset(int2 group_thread_id, int2 offset, min16float3 radiance, min16float roughness) {
    group_thread_id += offset;
    FFX_DNSR_Reflections_StoreInGroupSharedMemory(group_thread_id, radiance, roughness);
}

void FFX_DNSR_Reflections_InitializeGroupSharedMemory(int2 dispatch_thread_id, int2 group_thread_id) {
    int2 offset_0 = 0;
    if (group_thread_id.x < 4) {
        offset_0 = int2(8, 0);
    }
    else if (group_thread_id.y >= 4) {
        offset_0 = int2(4, 4);
    }
    else {
        offset_0 = -group_thread_id; // map all threads to the same memory location to guarantee cache hits.
    }

    int2 offset_1 = 0;
    if (group_thread_id.y < 4) {
        offset_1 = int2(0, 8);
    }
    else {
        offset_1 = -group_thread_id; // map all threads to the same memory location to guarantee cache hits.
    }

    min16float3 radiance_0;
    min16float roughness_0;

    min16float3 radiance_1;
    min16float roughness_1;

    min16float3 radiance_2;
    min16float roughness_2;

    /// XXA
    /// XXA
    /// BBC

    dispatch_thread_id -= 2;
    FFX_DNSR_Reflections_LoadWithOffset(dispatch_thread_id, int2(0, 0), radiance_0, roughness_0); // X
    FFX_DNSR_Reflections_LoadWithOffset(dispatch_thread_id, offset_0, radiance_1, roughness_1); // A & C
    FFX_DNSR_Reflections_LoadWithOffset(dispatch_thread_id, offset_1, radiance_2, roughness_2); // B
    
    FFX_DNSR_Reflections_StoreWithOffset(group_thread_id, int2(0, 0), radiance_0, roughness_0); // X
    if (group_thread_id.x < 4 || group_thread_id.y >= 4) {
        FFX_DNSR_Reflections_StoreWithOffset(group_thread_id, offset_0, radiance_1, roughness_1); // A & C
    }
    if (group_thread_id.y < 4) {
        FFX_DNSR_Reflections_StoreWithOffset(group_thread_id, offset_1, radiance_2, roughness_2); // B
    }
}

void FFX_DNSR_Reflections_Blur(int2 dispatch_thread_id, int2 group_thread_id, uint2 screen_size) {

    // First check if we have to denoise or if a simple copy is enough
    uint tile_meta_data_index = FFX_DNSR_Reflections_GetTileMetaDataIndex(dispatch_thread_id, screen_size.x);
    tile_meta_data_index = WaveReadLaneFirst(tile_meta_data_index);
    bool needs_denoiser = FFX_DNSR_Reflections_LoadTileMetaDataMask(tile_meta_data_index);

    [branch]
    if (needs_denoiser) {
        FFX_DNSR_Reflections_InitializeGroupSharedMemory(dispatch_thread_id, group_thread_id);
        GroupMemoryBarrierWithGroupSync();

        group_thread_id += 2; // Center threads in groupshared memory

        min16float3 center_radiance;
        min16float center_roughness;
        FFX_DNSR_Reflections_LoadFromGroupSharedMemory(group_thread_id, center_radiance, center_roughness);

        if (!FFX_DNSR_Reflections_IsGlossyReflection(center_roughness) || FFX_DNSR_Reflections_IsMirrorReflection(center_roughness)) {
            return;
        }

        min16float3 radiance = FFX_DNSR_Reflections_Resolve(group_thread_id, center_roughness, g_roughness_sigma_min, g_roughness_sigma_max);
        FFX_DNSR_Reflections_StoreDenoisedReflectionResult(dispatch_thread_id, radiance);
    }
}
#endif //FFX_DNSR_REFLECTIONS_BLUR
