// This is a modified version of the SSAO from Microsoft MiniEngine at https://github.com/Microsoft/DirectX-Graphics-Samples

#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

#ifdef INTERLEAVE_RESULT
TEXTURE2DARRAY(texture_lineardepth_deinterleaved, float, TEXSLOT_ONDEMAND0);
#else
TEXTURE2D(texture_lineardepth_interleaved, float, TEXSLOT_ONDEMAND0);
#define WIDE_SAMPLING 1
#endif // INTERLEAVE_RESULT

RWTEXTURE2D(output, unorm float, 0);

#if WIDE_SAMPLING
// 32x32 cache size:  the 16x16 in the center forms the area of focus with the 8-pixel perimeter used for wide gathering.
#define TILE_DIM 32
#define THREAD_COUNT_X 16
#define THREAD_COUNT_Y 16
#else
// 16x16 cache size:  the 8x8 in the center forms the area of focus with the 4-pixel perimeter used for gathering.
#define TILE_DIM 16
#define THREAD_COUNT_X 8
#define THREAD_COUNT_Y 8
#endif // WIDE_SAMPLING

groupshared float DepthSamples[TILE_DIM * TILE_DIM];

float TestSamplePair(float frontDepth, float invRange, uint base, int offset)
{
    // "Disocclusion" measures the penetration distance of the depth sample within the sphere.
    // Disocclusion < 0 (full occlusion) -> the sample fell in front of the sphere
    // Disocclusion > 1 (no occlusion) -> the sample fell behind the sphere
    float disocclusion1 = DepthSamples[base + offset] * invRange - frontDepth;
    float disocclusion2 = DepthSamples[base - offset] * invRange - frontDepth;

    float pseudoDisocclusion1 = saturate(xRejectFadeoff * disocclusion1);
    float pseudoDisocclusion2 = saturate(xRejectFadeoff * disocclusion2);

    return saturate(
        clamp(disocclusion1, pseudoDisocclusion2, 1.0) +
        clamp(disocclusion2, pseudoDisocclusion1, 1.0) -
        pseudoDisocclusion1 * pseudoDisocclusion2);
}

float TestSamples(uint centerIdx, uint x, uint y, float invDepth, float invThickness)
{
#if WIDE_SAMPLING
    x <<= 1;
    y <<= 1;
#endif // WIDE_SAMPLING

    float invRange = invThickness * invDepth;
    float frontDepth = invThickness - 0.5;

    if (y == 0)
    {
        // Axial
        return 0.5 * (
            TestSamplePair(frontDepth, invRange, centerIdx, x) +
            TestSamplePair(frontDepth, invRange, centerIdx, x * TILE_DIM));
    }
    else if (x == y)
    {
        // Diagonal
        return 0.5 * (
            TestSamplePair(frontDepth, invRange, centerIdx, x * TILE_DIM - x) +
            TestSamplePair(frontDepth, invRange, centerIdx, x * TILE_DIM + x));
    }
    else
    {
        // L-Shaped
        return 0.25 * (
            TestSamplePair(frontDepth, invRange, centerIdx, y * TILE_DIM + x) +
            TestSamplePair(frontDepth, invRange, centerIdx, y * TILE_DIM - x) +
            TestSamplePair(frontDepth, invRange, centerIdx, x * TILE_DIM + y) +
            TestSamplePair(frontDepth, invRange, centerIdx, x * TILE_DIM - y));
    }
}

[numthreads(THREAD_COUNT_X, THREAD_COUNT_Y, 1)]
void main(uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
#if WIDE_SAMPLING
    float2 QuadCenterUV = int2(DTid.xy + GTid.xy - 7) * xInvSliceDimension.xy;
#else
    float2 QuadCenterUV = int2(DTid.xy + GTid.xy - 3) * xInvSliceDimension.xy;
#endif // WIDE_SAMPLING

    // Fetch four depths and store them in LDS
#ifdef INTERLEAVE_RESULT
    float4 depths = texture_lineardepth_deinterleaved.Gather(sampler_linear_clamp, float3(QuadCenterUV, DTid.z));
#else
    float4 depths = texture_lineardepth_interleaved.Gather(sampler_linear_clamp, QuadCenterUV);
#endif // INTERLEAVE_RESULT

    int destIdx = GTid.x * 2 + GTid.y * 2 * TILE_DIM;
    DepthSamples[destIdx] = depths.w;
    DepthSamples[destIdx + 1] = depths.z;
    DepthSamples[destIdx + TILE_DIM] = depths.x;
    DepthSamples[destIdx + TILE_DIM + 1] = depths.y;

    GroupMemoryBarrierWithGroupSync();

#if WIDE_SAMPLING
    uint thisIdx = GTid.x + GTid.y * TILE_DIM + 8 * TILE_DIM + 8;
#else
    uint thisIdx = GTid.x + GTid.y * TILE_DIM + 4 * TILE_DIM + 4;
#endif // WIDE_SAMPLING
    const float invThisDepth = 1.0 / DepthSamples[thisIdx];

    float ao = 0.0;

#ifdef MSAO_SAMPLE_EXHAUSTIVELY
    // 68 samples:  sample all cells in *within* a circular radius of 5
    ao += xSampleWeightTable[0].x * TestSamples(thisIdx, 1, 0, invThisDepth, xInvThicknessTable[0].x);
    ao += xSampleWeightTable[0].y * TestSamples(thisIdx, 2, 0, invThisDepth, xInvThicknessTable[0].y);
    ao += xSampleWeightTable[0].z * TestSamples(thisIdx, 3, 0, invThisDepth, xInvThicknessTable[0].z);
    ao += xSampleWeightTable[0].w * TestSamples(thisIdx, 4, 0, invThisDepth, xInvThicknessTable[0].w);
    ao += xSampleWeightTable[1].x * TestSamples(thisIdx, 1, 1, invThisDepth, xInvThicknessTable[1].x);
    ao += xSampleWeightTable[2].x * TestSamples(thisIdx, 2, 2, invThisDepth, xInvThicknessTable[2].x);
    ao += xSampleWeightTable[2].w * TestSamples(thisIdx, 3, 3, invThisDepth, xInvThicknessTable[2].w);
    ao += xSampleWeightTable[1].y * TestSamples(thisIdx, 1, 2, invThisDepth, xInvThicknessTable[1].y);
    ao += xSampleWeightTable[1].z * TestSamples(thisIdx, 1, 3, invThisDepth, xInvThicknessTable[1].z);
    ao += xSampleWeightTable[1].w * TestSamples(thisIdx, 1, 4, invThisDepth, xInvThicknessTable[1].w);
    ao += xSampleWeightTable[2].y * TestSamples(thisIdx, 2, 3, invThisDepth, xInvThicknessTable[2].y);
    ao += xSampleWeightTable[2].z * TestSamples(thisIdx, 2, 4, invThisDepth, xInvThicknessTable[2].z);
#else // SAMPLE_CHECKER
    // 36 samples:  sample every-other cell in a checker board pattern
    ao += xSampleWeightTable[0].y * TestSamples(thisIdx, 2, 0, invThisDepth, xInvThicknessTable[0].y);
    ao += xSampleWeightTable[0].w * TestSamples(thisIdx, 4, 0, invThisDepth, xInvThicknessTable[0].w);
    ao += xSampleWeightTable[1].x * TestSamples(thisIdx, 1, 1, invThisDepth, xInvThicknessTable[1].x);
    ao += xSampleWeightTable[2].x * TestSamples(thisIdx, 2, 2, invThisDepth, xInvThicknessTable[2].x);
    ao += xSampleWeightTable[2].w * TestSamples(thisIdx, 3, 3, invThisDepth, xInvThicknessTable[2].w);
    ao += xSampleWeightTable[1].z * TestSamples(thisIdx, 1, 3, invThisDepth, xInvThicknessTable[1].z);
    ao += xSampleWeightTable[2].z * TestSamples(thisIdx, 2, 4, invThisDepth, xInvThicknessTable[2].z);
#endif // MSAO_SAMPLE_EXHAUSTIVELY

#ifdef INTERLEAVE_RESULT
    uint2 OutPixel = DTid.xy << 2 | uint2(DTid.z & 3, DTid.z >> 2);
#else
    uint2 OutPixel = DTid.xy;
#endif // INTERLEAVE_RESULT
    output[OutPixel] = ao * xRcpAccentuation;
}
