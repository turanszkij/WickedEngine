#include "globals.hlsli"
#include "brdf.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> texture_rayIndirectSpecular : register(t0);
Texture2D<float4> texture_rayDirectionPDF : register(t1);
Texture2D<float> texture_rayLength : register(t2);

RWTexture2D<float4> texture_resolve : register(u0);
RWTexture2D<float> texture_resolve_variance : register(u1);
RWTexture2D<float> texture_reprojectionDepth : register(u2);

static const float2 resolveSpatialSizeMinMax = float2(2.0, 8.0); // Good to have a min size as downsample scale (2x in this case)
static const uint resolveSpatialReconstructionCount = 4.0f;

float GetWeight(int2 neighborTracingCoord, float3 V, float3 N, float roughness, float NdotV)
{
	// Sample local pixel information
	float4 rayDirectionPDF = texture_rayDirectionPDF[neighborTracingCoord];
	float3 rayDirection = rayDirectionPDF.rgb;
	float PDF = rayDirectionPDF.a;

	float3 sampleL = normalize(rayDirection);
	float3 sampleH = normalize(sampleL + V);

	float sampleNdotH = saturate(dot(N, sampleH));
	float sampleNdotL = saturate(dot(N, sampleL));
	
	float roughnessBRDF = sqr(clamp(roughness, 0.045, 1));

	float Vis = V_SmithGGXCorrelated(roughnessBRDF, NdotV, sampleNdotL);
	float D = D_GGX(roughnessBRDF, sampleNdotH, sampleH);
	float localBRDF = Vis * D * sampleNdotL;

	float weight = localBRDF / max(PDF, 0.00001f);

	return weight;
}

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
	const float roughness = texture_roughness[DTid.xy];

	if (!NeedReflection(roughness, depth, ssr_roughness_cutoff))
	{
		texture_resolve[DTid.xy] = texture_rayIndirectSpecular[tracingCoord];
		texture_resolve_variance[DTid.xy] = 0.0;
		texture_reprojectionDepth[DTid.xy] = 0.0;
		return;
	}

	// Everthing in world space:
	const float3 P = reconstruct_position(uv, depth);
	const float3 N = decode_oct(texture_normal[DTid.xy]);
	const float3 V = normalize(GetCamera().position - P);
	const float NdotV = saturate(dot(N, V));

	const float resolveSpatialScale = saturate(roughness * 5.0); // roughness 0.2 is destination
	const float2 resolveSpatialSize = lerp(resolveSpatialSizeMinMax.x, resolveSpatialSizeMinMax.y, resolveSpatialScale);

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

		float neighborDepth = texture_depth[neighborCoord];
		if (neighborDepth > 0.0)
		{
			float weight = GetWeight(neighborTracingCoord, V, N, roughness, NdotV);

			float4 sampleColor = texture_rayIndirectSpecular[neighborTracingCoord];
			sampleColor.rgb *= rcp(1 + Luminance(sampleColor.rgb));

			result += sampleColor * weight;
			weightSum += weight;

			GetWeightedVariance(sampleColor, weight, weightSum, mean, S);

			if (weight > 0.001)
			{
				float neighborRayLength = texture_rayLength[neighborTracingCoord];
				closestRayLength = max(closestRayLength, neighborRayLength);
			}
		}
	}

	result /= weightSum;
	result.rgb *= rcp(1 - Luminance(result.rgb));

	// Population variance
	float resolveVariance = S / weightSum;

	// Convert to post-projection depth so we can construct dual source reprojection buffers later
	const float lineardepth = texture_lineardepth[DTid.xy] * GetCamera().z_far;
	float reprojectionDepth = compute_inverse_lineardepth(lineardepth + closestRayLength, GetCamera().z_near, GetCamera().z_far);

	texture_resolve[DTid.xy] = max(result, 0.00001f);
	texture_resolve_variance[DTid.xy] = resolveVariance;
	texture_reprojectionDepth[DTid.xy] = reprojectionDepth;
}
