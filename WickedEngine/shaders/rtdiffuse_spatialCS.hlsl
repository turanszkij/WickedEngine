#include "globals.hlsli"
#include "brdf.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> texture_rayIndirectDiffuse : register(t0);
Texture2D<float4> texture_rayDirectionPDF : register(t1);
Texture2D<float> texture_rayLength : register(t2);

RWTexture2D<float4> texture_resolve : register(u0);
RWTexture2D<float> texture_resolve_variance : register(u1);
RWTexture2D<float> texture_reprojectionDepth : register(u2);

static const float2 resolveSpatialSize = 14.0;
static const uint resolveSpatialReconstructionCount = 4.0f;

// Weighted incremental variance
// https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
void GetWeightedVariance(float4 sampleColor, float weight, float weightSum, inout float mean, inout float S)
{
	float luminance = Luminance(sampleColor.rgb);
	float oldMean = mean;
	mean += weight / weightSum * (luminance - oldMean);
	S += weight * (luminance - oldMean) * (luminance - mean);
}

// modified from 'globals.hlsli' with random shift
//	idx	: iteration index
//	num	: number of iterations in total
//  random : 16 bit random sequence
inline float2 hammersley2d_random(uint idx, uint num, uint2 random)
{
	uint bits = idx;
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	const float radicalInverse_VdC = float(bits ^ random.y) * 2.3283064365386963e-10; // / 0x100000000

	// ... & 0xffff) / (1 << 16): limit to 65536 then range 0 - 1
	return float2(frac(float(idx) / float(num) + float(random.x & 0xffff) / (1 << 16)), radicalInverse_VdC); // frac since we only want range [0; 1[
}

uint baseHash(uint3 p)
{
	p = 1103515245u * ((p.xyz >> 1u) ^ (p.yzx));
	uint h32 = 1103515245u * ((p.x ^ p.z) ^ (p.y >> 3u));
	return h32 ^ (h32 >> 16);
}

// Great quality hash with 3D input
// based on: https://www.shadertoy.com/view/Xt3cDn
uint3 hash33(uint3 x)
{
	uint n = baseHash(x);
	return uint3(n, n * 16807u, n * 48271u); //see: http://random.mat.sbg.ac.at/results/karl/server/node4.html
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;
	const uint2 tracingCoord = DTid.xy / 2;

	const float depth = texture_depth[DTid.xy];

	const float farplane = GetCamera().z_far;
	const float lineardepth = texture_lineardepth[DTid.xy] * farplane;

	// Everthing in world space:
	const float3 P = reconstruct_position(uv, depth);
	const float3 N = decode_oct(texture_normal[DTid.xy]);
	const float3 V = normalize(GetCamera().position - P);
	const float NdotV = saturate(dot(N, V));

	float4 result = 0.0f;
	float weightSum = 0.0f;

	float mean = 0.0f;
	float S = 0.0f;

	float closestRayLength = 0.0f;

	const uint sampleCount = resolveSpatialReconstructionCount;
	const uint2 random = hash33(uint3(DTid.xy, GetFrame().frame_count)).xy;

	for (int i = 0; i < sampleCount; i++)
	{
		float2 offset = (hammersley2d_random(i, sampleCount, random) - 0.5) * resolveSpatialSize;

		int2 neighborTracingCoord = tracingCoord + offset;
		int2 neighborCoord = DTid.xy + offset;

		float neighbor_lineardepth = texture_lineardepth[neighborCoord] * farplane;
		if (neighbor_lineardepth > 0)
		{
			float3 rayDir = texture_rayDirectionPDF[neighborTracingCoord].xyz;
			float weight = abs(dot(N, rayDir));
			//weight = 1;
			//weight *= 1 - saturate(abs(neighbor_lineardepth - lineardepth));

			float4 sampleColor = texture_rayIndirectDiffuse[neighborTracingCoord];
			sampleColor.rgb *= rcp(1 + Luminance(sampleColor.rgb));

			result += sampleColor * weight;
			weightSum += weight;

			GetWeightedVariance(sampleColor, weight, weightSum, mean, S);
		}
	}

	result /= weightSum;
	result.rgb *= rcp(1 - Luminance(result.rgb));

	// Population variance
	float resolveVariance = S / weightSum;

	// Convert to post-projection depth so we can construct dual source reprojection buffers later
	float reprojectionDepth = compute_inverse_lineardepth(lineardepth + closestRayLength, GetCamera().z_near, GetCamera().z_far);

	texture_resolve[DTid.xy] = max(result, 0.00001f);
	texture_resolve_variance[DTid.xy] = resolveVariance;
	texture_reprojectionDepth[DTid.xy] = reprojectionDepth;
}
