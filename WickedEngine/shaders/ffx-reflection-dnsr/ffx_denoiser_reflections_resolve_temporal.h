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

#ifndef FFX_DNSR_REFLECTIONS_RESOLVE_TEMPORAL
#define FFX_DNSR_REFLECTIONS_RESOLVE_TEMPORAL

#include "ffx_denoiser_reflections_common.h"

// From "Temporal Reprojection Anti-Aliasing"
// https://github.com/playdeadgames/temporal
/**********************************************************************
Copyright (c) [2015] [Playdead]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
********************************************************************/
float3 FFX_DNSR_Reflections_ClipAABB(float3 aabb_min, float3 aabb_max, float3 prev_sample) {
    // Main idea behind clipping - it prevents clustering when neighbor color space
    // is distant from history sample

    // Here we find intersection between color vector and aabb color box

    // Note: only clips towards aabb center
    float3 aabb_center = 0.5 * (aabb_max + aabb_min);
    float3 extent_clip = 0.5 * (aabb_max - aabb_min) + 0.001;

    // Find color vector
    float3 color_vector = prev_sample - aabb_center;
    // Transform into clip space
    float3 color_vector_clip = color_vector / extent_clip;
    // Find max absolute component
    color_vector_clip = abs(color_vector_clip);
    float max_abs_unit = max(max(color_vector_clip.x, color_vector_clip.y), color_vector_clip.z);

    if (max_abs_unit > 1.0) {
        return aabb_center + color_vector / max_abs_unit; // clip towards color vector
    }
    else {
        return prev_sample; // point is inside aabb
    }
}

// Estimates spatial reflection radiance standard deviation
float3 FFX_DNSR_Reflections_EstimateLocalNeighborhood(int2 dispatch_thread_id) {
    float3 color_sum = 0.0;
    float3 color_sum_squared = 0.0;

    int radius = 1;
    float weight = (radius * 2.0 + 1.0) * (radius * 2.0 + 1.0);

    for (int dx = -radius; dx <= radius; dx++) {
        for (int dy = -radius; dy <= radius; dy++) {
            int2 texel_coords = dispatch_thread_id + int2(dx, dy);
            float3 value = FFX_DNSR_Reflections_LoadSpatiallyDenoisedReflections(texel_coords);
            color_sum += value;
            color_sum_squared += value * value;
        }
    }

    float3 color_std = (color_sum_squared - color_sum * color_sum / weight) / (weight - 1.0);
    return sqrt(max(color_std, 0.0));
}

float2 FFX_DNSR_Reflections_GetSurfaceReprojection(int2 dispatch_thread_id, float2 uv, float2 motion_vector) {
    // Reflector position reprojection
    float2 history_uv = uv - motion_vector;
    return history_uv;
}

float2 FFX_DNSR_Reflections_GetHitPositionReprojection(int2 dispatch_thread_id, float2 uv, float reflected_ray_length) {
    float z = FFX_DNSR_Reflections_LoadDepth(dispatch_thread_id);
    float3 view_space_ray = FFX_DNSR_Reflections_ScreenSpaceToViewSpace(float3(uv, z));

    // We start out with reconstructing the ray length in view space. 
    // This includes the portion from the camera to the reflecting surface as well as the portion from the surface to the hit position.
    float surface_depth = length(view_space_ray);
    float ray_length = surface_depth + reflected_ray_length;

    // We then perform a parallax correction by shooting a ray 
    // of the same length "straight through" the reflecting surface
    // and reprojecting the tip of that ray to the previous frame.
    view_space_ray /= surface_depth; // == normalize(view_space_ray)
    view_space_ray *= ray_length;
    float3 world_hit_position = FFX_DNSR_Reflections_ViewSpaceToWorldSpace(float4(view_space_ray, 1)); // This is the "fake" hit position if we would follow the ray straight through the surface.
    float3 prev_hit_position = FFX_DNSR_Reflections_WorldSpaceToScreenSpacePrevious(world_hit_position);
    float2 history_uv = prev_hit_position.xy;
    return history_uv;
}

float FFX_DNSR_Reflections_SampleHistory(float2 uv, uint2 screen_size, float3 normal, float roughness, float3 radiance_min, float3 radiance_max, float roughness_sigma_min, float roughness_sigma_max,  float temporalStabilityFactor, out float3 radiance) {
    int2 texel_coords = int2(screen_size * uv);
    radiance = FFX_DNSR_Reflections_LoadRadianceHistory(texel_coords);
    radiance = FFX_DNSR_Reflections_ClipAABB(radiance_min, radiance_max, radiance);

    float3 history_normal =   FFX_DNSR_Reflections_LoadNormalHistory(texel_coords);
    float history_roughness = FFX_DNSR_Reflections_LoadRoughnessHistory(texel_coords);

    const float normal_sigma = 8.0;

    float accumulation_speed = temporalStabilityFactor
        * FFX_DNSR_Reflections_GetEdgeStoppingNormalWeight(normal, history_normal, normal_sigma)
        * FFX_DNSR_Reflections_GetEdgeStoppingRoughnessWeight(roughness, history_roughness, roughness_sigma_min, roughness_sigma_max)
        * FFX_DNSR_Reflections_GetRoughnessAccumulationWeight(roughness)
        ;

    return saturate(accumulation_speed);
}

float FFX_DNSR_Reflections_ComputeTemporalVariance(float3 history_radiance, float3 radiance) {
    // Check temporal variance. 
    float history_luminance = FFX_DNSR_Reflections_Luminance(history_radiance);
    float luminance = FFX_DNSR_Reflections_Luminance(radiance);
    return abs(history_luminance - luminance) / max(max(history_luminance, luminance), 0.00001);
}

groupshared uint g_FFX_DNSR_Reflections_TemporalVarianceMask[2];

void FFX_DNSR_Reflections_WriteTemporalVarianceMask(uint mask_write_index, uint has_temporal_variance_mask) {
    // All lanes write to the same index, so we combine the masks within the wave and do a single write
    const uint s_has_temporal_variance_mask = WaveActiveBitOr(has_temporal_variance_mask);
    if (WaveIsFirstLane()) {
        FFX_DNSR_Reflections_StoreTemporalVarianceMask(mask_write_index, s_has_temporal_variance_mask);
    }
}

void FFX_DNSR_Reflections_WriteTemporalVariance(int2 dispatch_thread_id, int2 group_thread_id, uint2 screen_size, bool has_temporal_variance) {
    uint mask_write_index = FFX_DNSR_Reflections_GetTemporalVarianceIndex(dispatch_thread_id, screen_size.x);
    uint lane_mask = FFX_DNSR_Reflections_GetBitMaskFromPixelPosition(dispatch_thread_id);
    uint has_temporal_variance_mask = has_temporal_variance ? lane_mask : 0;

    if (WaveGetLaneCount() == 32) {
        FFX_DNSR_Reflections_WriteTemporalVarianceMask(mask_write_index, has_temporal_variance_mask);
    }
    else if (WaveGetLaneCount() == 64) { // The lower 32 lanes write to a different index than the upper 32 lanes.
        if (WaveGetLaneIndex() < 32) {
            FFX_DNSR_Reflections_WriteTemporalVarianceMask(mask_write_index, has_temporal_variance_mask); // Write lower
        }
        else {
            FFX_DNSR_Reflections_WriteTemporalVarianceMask(mask_write_index, has_temporal_variance_mask); // Write upper
        }
    }
    else { // Use groupshared memory for all other wave sizes
        uint mask_index = group_thread_id.y / 4;
        g_FFX_DNSR_Reflections_TemporalVarianceMask[mask_index] = 0;
        GroupMemoryBarrierWithGroupSync();
        InterlockedOr(g_FFX_DNSR_Reflections_TemporalVarianceMask[mask_index], has_temporal_variance_mask);
        GroupMemoryBarrierWithGroupSync();

        if (all(group_thread_id == 0)) {
            FFX_DNSR_Reflections_StoreTemporalVarianceMask(mask_write_index, g_FFX_DNSR_Reflections_TemporalVarianceMask[0]);
            FFX_DNSR_Reflections_StoreTemporalVarianceMask(mask_write_index + 1, g_FFX_DNSR_Reflections_TemporalVarianceMask[1]);
        }
    }
}

void FFX_DNSR_Reflections_ResolveTemporal(int2 dispatch_thread_id, int2 group_thread_id, uint2 screen_size, float temporal_stability_factor, float temporal_variance_threshold) {

    // First check if we have to denoise or if a simple copy is enough
    uint tile_meta_data_index = FFX_DNSR_Reflections_GetTileMetaDataIndex(dispatch_thread_id, screen_size.x);
    tile_meta_data_index = WaveReadLaneFirst(tile_meta_data_index);
    bool needs_denoiser = FFX_DNSR_Reflections_LoadTileMetaDataMask(tile_meta_data_index);

    bool has_temporal_variance = false;

    [branch]
    if (needs_denoiser) {
        float roughness = FFX_DNSR_Reflections_LoadRoughness(dispatch_thread_id);
        if (!FFX_DNSR_Reflections_IsGlossyReflection(roughness) || FFX_DNSR_Reflections_IsMirrorReflection(roughness)) {
            return;
        }

        float2 uv = float2(dispatch_thread_id.x + 0.5, dispatch_thread_id.y + 0.5) / screen_size;

        float3 normal = FFX_DNSR_Reflections_LoadNormal(dispatch_thread_id);
        float3 radiance = FFX_DNSR_Reflections_LoadSpatiallyDenoisedReflections(dispatch_thread_id);
        float3 radiance_history = FFX_DNSR_Reflections_LoadRadianceHistory(dispatch_thread_id);
        float ray_length = FFX_DNSR_Reflections_LoadRayLength(dispatch_thread_id);

        // And clip it to the local neighborhood
        float2 motion_vector = FFX_DNSR_Reflections_LoadMotionVector(dispatch_thread_id);
        float3 color_std = FFX_DNSR_Reflections_EstimateLocalNeighborhood(dispatch_thread_id);
        color_std *= 2.2;

        float3 radiance_min = radiance.xyz - color_std;
        float3 radiance_max = radiance + color_std;

        // Reproject point on the reflecting surface
        float2 surface_reprojection_uv = FFX_DNSR_Reflections_GetSurfaceReprojection(dispatch_thread_id, uv, motion_vector);

        // Reproject hit point
        float2 hit_reprojection_uv = FFX_DNSR_Reflections_GetHitPositionReprojection(dispatch_thread_id, uv, ray_length);

        float2 reprojection_uv;
        reprojection_uv = (roughness < 0.05) ? hit_reprojection_uv : surface_reprojection_uv;

        float3 reprojection = 0;
        float weight = 0;
        if (all(reprojection_uv > 0.0) && all(reprojection_uv < 1.0)) {
            weight = FFX_DNSR_Reflections_SampleHistory(reprojection_uv, screen_size, normal, roughness, radiance_min, radiance_max, g_roughness_sigma_min, g_roughness_sigma_max, temporal_stability_factor, reprojection);
        }

        radiance = lerp(radiance, reprojection, weight);
        has_temporal_variance = FFX_DNSR_Reflections_ComputeTemporalVariance(radiance_history, radiance) > temporal_variance_threshold;

        FFX_DNSR_Reflections_StoreTemporallyDenoisedReflections(dispatch_thread_id, radiance);
    }

    FFX_DNSR_Reflections_WriteTemporalVariance(dispatch_thread_id, group_thread_id, screen_size, has_temporal_variance);
}

#endif //FFX_DNSR_REFLECTIONS_RESOLVE_TEMPORAL
