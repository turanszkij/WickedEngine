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

#ifndef FFX_DNSR_SHADOWS_FILTER_HLSL
#define FFX_DNSR_SHADOWS_FILTER_HLSL

#include "ffx_denoiser_shadows_util.h"

groupshared uint g_FFX_DNSR_Shadows_shared_input[16][16];
groupshared float g_FFX_DNSR_Shadows_shared_depth[16][16];
groupshared uint g_FFX_DNSR_Shadows_shared_normals_xy[16][16];
groupshared uint g_FFX_DNSR_Shadows_shared_normals_zw[16][16];

uint FFX_DNSR_Shadows_PackFloat16(float16_t2 v)
{
    uint2 p = f32tof16(float2(v));
    return p.x | (p.y << 16);
}

float16_t2 FFX_DNSR_Shadows_UnpackFloat16(uint a)
{
    float2 tmp = f16tof32(
        uint2(a & 0xFFFF, a >> 16));
    return float16_t2(tmp);
}

float16_t2 FFX_DNSR_Shadows_LoadInputFromGroupSharedMemory(int2 idx)
{
    return FFX_DNSR_Shadows_UnpackFloat16(g_FFX_DNSR_Shadows_shared_input[idx.y][idx.x]);
}

float FFX_DNSR_Shadows_LoadDepthFromGroupSharedMemory(int2 idx)
{
    return g_FFX_DNSR_Shadows_shared_depth[idx.y][idx.x];
}

float16_t3 FFX_DNSR_Shadows_LoadNormalsFromGroupSharedMemory(int2 idx)
{
    float16_t3 normals;
    normals.xy = FFX_DNSR_Shadows_UnpackFloat16(g_FFX_DNSR_Shadows_shared_normals_xy[idx.y][idx.x]);
    normals.z = FFX_DNSR_Shadows_UnpackFloat16(g_FFX_DNSR_Shadows_shared_normals_zw[idx.y][idx.x]).x;
    return normals;
}

void FFX_DNSR_Shadows_StoreInGroupSharedMemory(int2 idx, float16_t3 normals, float16_t2 input, float depth)
{
    g_FFX_DNSR_Shadows_shared_input[idx.y][idx.x] = FFX_DNSR_Shadows_PackFloat16(input);
    g_FFX_DNSR_Shadows_shared_depth[idx.y][idx.x] = depth;
    g_FFX_DNSR_Shadows_shared_normals_xy[idx.y][idx.x] = FFX_DNSR_Shadows_PackFloat16(normals.xy);
    g_FFX_DNSR_Shadows_shared_normals_zw[idx.y][idx.x] = FFX_DNSR_Shadows_PackFloat16(float16_t2(normals.z, 0));
}

void FFX_DNSR_Shadows_LoadWithOffset(int2 did, int2 offset, out float16_t3 normals, out float16_t2 input, out float depth)
{
    did += offset;

    const int2 p = clamp(did, int2(0, 0), FFX_DNSR_Shadows_GetBufferDimensions() - 1);
    normals = FFX_DNSR_Shadows_ReadNormals(p);
    input = FFX_DNSR_Shadows_ReadInput(p);
    depth = FFX_DNSR_Shadows_ReadDepth(p);
}

void FFX_DNSR_Shadows_StoreWithOffset(int2 gtid, int2 offset, float16_t3 normals, float16_t2 input, float depth)
{
    gtid += offset;
    FFX_DNSR_Shadows_StoreInGroupSharedMemory(gtid, normals, input, depth);
}

void FFX_DNSR_Shadows_InitializeGroupSharedMemory(int2 did, int2 gtid)
{
    int2 offset_0 = 0;
    int2 offset_1 = int2(8, 0);
    int2 offset_2 = int2(0, 8);
    int2 offset_3 = int2(8, 8);

    float16_t3 normals_0;
    float16_t2 input_0;
    float depth_0;

    float16_t3 normals_1;
    float16_t2 input_1;
    float depth_1;

    float16_t3 normals_2;
    float16_t2 input_2;
    float depth_2;

    float16_t3 normals_3;
    float16_t2 input_3;
    float depth_3;

    /// XA
    /// BC

    did -= 4;
    FFX_DNSR_Shadows_LoadWithOffset(did, offset_0, normals_0, input_0, depth_0); // X
    FFX_DNSR_Shadows_LoadWithOffset(did, offset_1, normals_1, input_1, depth_1); // A
    FFX_DNSR_Shadows_LoadWithOffset(did, offset_2, normals_2, input_2, depth_2); // B
    FFX_DNSR_Shadows_LoadWithOffset(did, offset_3, normals_3, input_3, depth_3); // C

    FFX_DNSR_Shadows_StoreWithOffset(gtid, offset_0, normals_0, input_0, depth_0); // X
    FFX_DNSR_Shadows_StoreWithOffset(gtid, offset_1, normals_1, input_1, depth_1); // A
    FFX_DNSR_Shadows_StoreWithOffset(gtid, offset_2, normals_2, input_2, depth_2); // B
    FFX_DNSR_Shadows_StoreWithOffset(gtid, offset_3, normals_3, input_3, depth_3); // C
}

float FFX_DNSR_Shadows_GetShadowSimilarity(float x1, float x2, float sigma)
{
    return exp(-abs(x1 - x2) / sigma);
}

float FFX_DNSR_Shadows_GetDepthSimilarity(float x1, float x2, float sigma)
{
    return exp(-abs(x1 - x2) / sigma);
}

float FFX_DNSR_Shadows_GetNormalSimilarity(float3 x1, float3 x2)
{
    return pow(saturate(dot(x1, x2)), 32.0f);
}

float FFX_DNSR_Shadows_GetLinearDepth(uint2 did, float depth)
{
    const float2 uv = (did + 0.5f) * FFX_DNSR_Shadows_GetInvBufferDimensions();
    const float2 ndc = 2.0f * float2(uv.x, 1.0f - uv.y) - 1.0f;

    float4 projected = mul(FFX_DNSR_Shadows_GetProjectionInverse(), float4(ndc, depth, 1));
    return abs(projected.z / projected.w);
}

float FFX_DNSR_Shadows_FetchFilteredVarianceFromGroupSharedMemory(int2 pos)
{
    const int k = 1;
    float variance = 0.0f;
    const float kernel[2][2] =
    {
        { 1.0f / 4.0f, 1.0f / 8.0f  },
        { 1.0f / 8.0f, 1.0f / 16.0f }
    };
    for (int y = -k; y <= k; ++y)
    {
        for (int x = -k; x <= k; ++x)
        {
            const float w = kernel[abs(x)][abs(y)];
            variance += w * FFX_DNSR_Shadows_LoadInputFromGroupSharedMemory(pos + int2(x, y)).y;
        }
    }
    return variance;
}

void FFX_DNSR_Shadows_DenoiseFromGroupSharedMemory(uint2 did, uint2 gtid, inout float weight_sum, inout float2 shadow_sum, float depth, uint stepsize)
{
    // Load our center sample
    const float2 shadow_center = FFX_DNSR_Shadows_LoadInputFromGroupSharedMemory(gtid);
    const float3 normal_center = FFX_DNSR_Shadows_LoadNormalsFromGroupSharedMemory(gtid);

    weight_sum = 1.0f;
    shadow_sum = shadow_center;

    const float variance = FFX_DNSR_Shadows_FetchFilteredVarianceFromGroupSharedMemory(gtid);
    const float std_deviation = sqrt(max(variance + 1e-9f, 0.0f));
    const float depth_center = FFX_DNSR_Shadows_GetLinearDepth(did, depth);    // linearize the depth value

    // Iterate filter kernel
    const int k = 1;
    const float kernel[3] = { 1.0f, 2.0f / 3.0f, 1.0f / 6.0f };

    for (int y = -k; y <= k; ++y)
    {
        for (int x = -k; x <= k; ++x)
        {
            // Should we process this sample?
            const int2 step = int2(x, y) * stepsize;
            const int2 gtid_idx = gtid + step;
            const int2 did_idx = did + step;

            float depth_neigh = FFX_DNSR_Shadows_LoadDepthFromGroupSharedMemory(gtid_idx);
            float3 normal_neigh = FFX_DNSR_Shadows_LoadNormalsFromGroupSharedMemory(gtid_idx);
            float2 shadow_neigh = FFX_DNSR_Shadows_LoadInputFromGroupSharedMemory(gtid_idx);

            float sky_pixel_multiplier = ((x == 0 && y == 0) || depth_neigh >= 1.0f || depth_neigh <= 0.0f) ? 0 : 1; // Zero weight for sky pixels

            // Fetch our filtering values
            depth_neigh = FFX_DNSR_Shadows_GetLinearDepth(did_idx, depth_neigh);

            // Evaluate the edge-stopping function
            float w = kernel[abs(x)] * kernel[abs(y)];  // kernel weight
            w *= FFX_DNSR_Shadows_GetShadowSimilarity(shadow_center.x, shadow_neigh.x, std_deviation);
            w *= FFX_DNSR_Shadows_GetDepthSimilarity(depth_center, depth_neigh, FFX_DNSR_Shadows_GetDepthSimilaritySigma());
            w *= FFX_DNSR_Shadows_GetNormalSimilarity(normal_center, normal_neigh);
            w *= sky_pixel_multiplier;

            // Accumulate the filtered sample
            shadow_sum += float2(w, w * w) * shadow_neigh;
            weight_sum += w;
        }
    }
}

float2 FFX_DNSR_Shadows_ApplyFilterWithPrecache(uint2 did, uint2 gtid, uint stepsize)
{
    float weight_sum = 1.0;
    float2 shadow_sum = 0.0;

    FFX_DNSR_Shadows_InitializeGroupSharedMemory(did, gtid);
    bool needs_denoiser = FFX_DNSR_Shadows_IsShadowReciever(did);
    GroupMemoryBarrierWithGroupSync();
    if (needs_denoiser)
    {
        float depth = FFX_DNSR_Shadows_ReadDepth(did);
        gtid += 4; // Center threads in groupshared memory
        FFX_DNSR_Shadows_DenoiseFromGroupSharedMemory(did, gtid, weight_sum, shadow_sum, depth, stepsize);
    }

    float mean = shadow_sum.x / weight_sum;
    float variance = shadow_sum.y / (weight_sum * weight_sum);
    return float2(mean, variance);
}

void FFX_DNSR_Shadows_ReadTileMetaData(uint2 gid, out bool is_cleared, out bool all_in_light)
{
    uint meta_data = FFX_DNSR_Shadows_ReadTileMetaData(gid.y * FFX_DNSR_Shadows_RoundedDivide(FFX_DNSR_Shadows_GetBufferDimensions().x, 8) + gid.x);
    is_cleared = meta_data & TILE_META_DATA_CLEAR_MASK;
    all_in_light = meta_data & TILE_META_DATA_LIGHT_MASK;
}


float2 FFX_DNSR_Shadows_FilterSoftShadowsPass(uint2 gid, uint2 gtid, uint2 did, out bool bWriteResults, uint const pass, uint const stepsize)
{
    bool is_cleared;
    bool all_in_light;
    FFX_DNSR_Shadows_ReadTileMetaData(gid, is_cleared, all_in_light);

    bWriteResults = false;
    float2 results = float2(0, 0);
    [branch]
    if (is_cleared)
    {
        if (pass != 1)
        {
            results.x = all_in_light ? 1.0 : 0.0;
            bWriteResults = true;
        }
    }
    else
    {
        results = FFX_DNSR_Shadows_ApplyFilterWithPrecache(did, gtid, stepsize);
        bWriteResults = true;
    }

    return results;
}

#endif
