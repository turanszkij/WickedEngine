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

#ifndef FFX_DNSR_REFLECTIONS_COMMON
#define FFX_DNSR_REFLECTIONS_COMMON

uint FFX_DNSR_Reflections_RoundedDivide(uint value, uint divisor) {
    return (value + divisor - 1) / divisor;
}

uint FFX_DNSR_Reflections_GetTileMetaDataIndex(uint2 pixel_pos, uint screen_width) {
    uint2 tile_index = uint2(pixel_pos.x / 8, pixel_pos.y / 8);
    uint flattened = tile_index.y * FFX_DNSR_Reflections_RoundedDivide(screen_width, 8) + tile_index.x;
    return flattened;
}

uint FFX_DNSR_Reflections_GetTemporalVarianceIndex(uint2 pixel_pos, uint screen_width) {
    uint2 tile_index = uint2(pixel_pos.x / 8, pixel_pos.y / 8);
    uint flattened = tile_index.y * FFX_DNSR_Reflections_RoundedDivide(screen_width, 8) + tile_index.x;
    return 2 * flattened + ((pixel_pos.y % 8) / 4); // Position upper and lower half next to each other
}

uint FFX_DNSR_Reflections_GetBitMaskFromPixelPosition(uint2 pixel_pos) {
    int lane_index = (pixel_pos.y % 4) * 8 + (pixel_pos.x % 8);
    return (1u << lane_index);
}

float FFX_DNSR_Reflections_GetEdgeStoppingNormalWeight(float3 normal_p, float3 normal_q, float sigma) {
    return pow(max(dot(normal_p, normal_q), 0.0), sigma);
}

float FFX_DNSR_Reflections_GetEdgeStoppingRoughnessWeight(float roughness_p, float roughness_q, float sigma_min, float sigma_max) {
    return 1.0 - smoothstep(sigma_min, sigma_max, abs(roughness_p - roughness_q));
}

min16float FFX_DNSR_Reflections_GetEdgeStoppingRoughnessWeightFP16(min16float roughness_p, min16float roughness_q, min16float sigma_min, min16float sigma_max) {
    return 1.0 - smoothstep(sigma_min, sigma_max, abs(roughness_p - roughness_q));
}

// Roughness weight to prevent ghosting on pure mirror reflections
float FFX_DNSR_Reflections_GetRoughnessAccumulationWeight(float roughness) {
    float near_singular_roughness = 0.00001;
    return smoothstep(0.0, near_singular_roughness, roughness);
}

float FFX_DNSR_Reflections_Gaussian(float x, float m, float sigma) {
    float a = length(x - m) / sigma;
    a *= a;
    return exp(-0.5 * a);
}

float FFX_DNSR_Reflections_Luminance(float3 color) {
    return max(dot(color, float3(0.299, 0.587, 0.114)), 0.00001);
}

uint FFX_DNSR_Reflections_BitfieldExtract(uint src, uint off, uint bits) {
    uint mask = (1u << bits) - 1; 
    return (src >> off) & mask; 
}

uint FFX_DNSR_Reflections_BitfieldInsert(uint src, uint ins, uint bits) { 
    uint mask = (1u << bits) - 1; 
    return (ins & mask) | (src & (~mask)); 
}

//  LANE TO 8x8 MAPPING
//  ===================
//  00 01 08 09 10 11 18 19 
//  02 03 0a 0b 12 13 1a 1b
//  04 05 0c 0d 14 15 1c 1d
//  06 07 0e 0f 16 17 1e 1f 
//  20 21 28 29 30 31 38 39 
//  22 23 2a 2b 32 33 3a 3b
//  24 25 2c 2d 34 35 3c 3d
//  26 27 2e 2f 36 37 3e 3f 
uint2 FFX_DNSR_Reflections_RemapLane8x8(uint lane) { 
    return uint2(FFX_DNSR_Reflections_BitfieldInsert(FFX_DNSR_Reflections_BitfieldExtract(lane, 2u, 3u), lane, 1u)
    , FFX_DNSR_Reflections_BitfieldInsert(FFX_DNSR_Reflections_BitfieldExtract(lane, 3u, 3u)
    , FFX_DNSR_Reflections_BitfieldExtract(lane, 1u, 2u), 2u));
}

#endif // FFX_DNSR_REFLECTIONS_COMMON
