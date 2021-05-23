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

#ifndef FFX_DNSR_REFLECTIONS_CLASSIFY_TILES
#define FFX_DNSR_REFLECTIONS_CLASSIFY_TILES

#include "ffx_denoiser_reflections_common.h"

bool FFX_DNSR_Reflections_IsBaseRay(uint2 dispatch_thread_id, uint samples_per_quad) {
    switch (samples_per_quad) {
    case 1:
        return ((dispatch_thread_id.x & 1) | (dispatch_thread_id.y & 1)) == 0; // Deactivates 3 out of 4 rays
    case 2:
        return (dispatch_thread_id.x & 1) == (dispatch_thread_id.y & 1); // Deactivates 2 out of 4 rays. Keeps diagonal.
    default: // case 4:
        return true;
    }
}

groupshared uint g_FFX_DNSR_TileCount;

void FFX_DNSR_Reflections_ClassifyTiles(uint2 dispatch_thread_id, uint2 group_thread_id, float roughness, uint2 screen_size, uint samples_per_quad, bool enable_temporal_variance_guided_tracing) {
    g_FFX_DNSR_TileCount = 0;

    bool is_first_lane_of_wave = WaveIsFirstLane();

    // First we figure out on a per thread basis if we need to shoot a reflection ray.
    // Disable offscreen pixels
    bool needs_ray = !(dispatch_thread_id.x >= screen_size.x || dispatch_thread_id.y >= screen_size.y);

    // Dont shoot a ray on very rough surfaces.
    needs_ray = needs_ray && FFX_DNSR_Reflections_IsGlossyReflection(roughness);

    // Also we dont need to run the denoiser on mirror reflections.
    bool needs_denoiser = needs_ray && !FFX_DNSR_Reflections_IsMirrorReflection(roughness);

    // Decide which ray to keep
    bool is_base_ray = FFX_DNSR_Reflections_IsBaseRay(dispatch_thread_id, samples_per_quad);
    needs_ray = needs_ray && (!needs_denoiser || is_base_ray); // Make sure to not deactivate mirror reflection rays.

    if (enable_temporal_variance_guided_tracing && needs_denoiser && !needs_ray) {
        uint lane_mask = FFX_DNSR_Reflections_GetBitMaskFromPixelPosition(dispatch_thread_id);
        uint base_mask_index = FFX_DNSR_Reflections_GetTemporalVarianceIndex(dispatch_thread_id & (~0b111), screen_size.x);
        base_mask_index = WaveReadLaneFirst(base_mask_index);

        uint temporal_variance_mask_upper = FFX_DNSR_Reflections_LoadTemporalVarianceMask(base_mask_index);
        uint temporal_variance_mask_lower = FFX_DNSR_Reflections_LoadTemporalVarianceMask(base_mask_index + 1);
        uint temporal_variance_mask = group_thread_id.y < 4 ? temporal_variance_mask_upper : temporal_variance_mask_lower;

        bool has_temporal_variance = temporal_variance_mask & lane_mask;
        needs_ray = needs_ray || has_temporal_variance;
    }

    GroupMemoryBarrierWithGroupSync(); // Wait until g_FFX_DNSR_TileCount is cleared - allow some computations before and after

    // Now we know for each thread if it needs to shoot a ray and wether or not a denoiser pass has to run on this pixel.
    
    // Next we have to figure out for which pixels that ray is creating the values for. Thus, if we have to copy its value horizontal, vertical or across.
    bool require_copy = !needs_ray && needs_denoiser; // Our pixel only requires a copy if we want to run a denoiser on it but don't want to shoot a ray for it.
    bool copy_horizontal = (samples_per_quad != 4) && is_base_ray && WaveReadLaneAt(require_copy, WaveGetLaneIndex() ^ 0b01); // QuadReadAcrossX
    bool copy_vertical = (samples_per_quad == 1) && is_base_ray && WaveReadLaneAt(require_copy, WaveGetLaneIndex() ^ 0b10); // QuadReadAcrossY
    bool copy_diagonal = (samples_per_quad == 1) && is_base_ray && WaveReadLaneAt(require_copy, WaveGetLaneIndex() ^ 0b11); // QuadReadAcrossDiagonal

    // Thus, we need to compact the rays and append them all at once to the ray list.
    uint local_ray_index_in_wave = WavePrefixCountBits(needs_ray);
    uint wave_ray_count = WaveActiveCountBits(needs_ray);
    uint base_ray_index;
    if (is_first_lane_of_wave) {
        FFX_DNSR_Reflections_IncrementRayCounter(wave_ray_count, base_ray_index);
    }
    base_ray_index = WaveReadLaneFirst(base_ray_index);
    if (needs_ray) {
        int ray_index = base_ray_index + local_ray_index_in_wave;
        FFX_DNSR_Reflections_StoreRay(ray_index, dispatch_thread_id, copy_horizontal, copy_vertical, copy_diagonal);
    }

    // Write tile meta data masks
    bool wave_needs_denoiser = WaveActiveAnyTrue(needs_denoiser);
    if (WaveIsFirstLane() && wave_needs_denoiser) {
        InterlockedAdd(g_FFX_DNSR_TileCount, 1);
    }

    GroupMemoryBarrierWithGroupSync(); // Wait until all waves wrote into g_FFX_DNSR_TileCount

    if (all(group_thread_id == 0)) {
        uint tile_meta_data_index = FFX_DNSR_Reflections_GetTileMetaDataIndex(WaveReadLaneFirst(dispatch_thread_id), screen_size.x);
        FFX_DNSR_Reflections_StoreTileMetaDataMask(tile_meta_data_index, g_FFX_DNSR_TileCount);
    }
}

#endif //FFX_DNSR_REFLECTIONS_CLASSIFY_TILES