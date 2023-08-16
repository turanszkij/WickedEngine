#include "globals.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> texture_temporal : register(t0);
Texture2D<float> texture_resolve_variance : register(t1);

RWTexture2D<float4> output : register(u0);

static const float depthThreshold = 10000.0;
static const float normalThreshold = 1.0;
static const float varianceEstimateThreshold = 0.015; // Larger variance values use stronger blur
static const float varianceExitThreshold = 0.005; // Variance needs to be higher than this value to accept blur
static const uint2 bilateralMinMaxRadius = uint2(0, 2); // Chosen by variance

#define BILATERAL_SIGMA 0.9

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
#if 0 // Debug
	output[DTid.xy] = float4((texture_resolve_variance[DTid.xy] > varianceEstimateThreshold).rrr, 1.0);
	return;
#endif

	const float depth = texture_depth[DTid.xy];
	const float roughness = texture_roughness[DTid.xy];

	if (!NeedReflection(roughness, depth, ssr_roughness_cutoff))
	{
		output[DTid.xy] = texture_temporal[DTid.xy];
		return;
	}

	float2 direction = postprocess.params0.xy;

	const float linearDepth = texture_lineardepth[DTid.xy];
	const float3 N = decode_oct(texture_normal[DTid.xy]);

	float4 outputColor = texture_temporal[DTid.xy];


	float variance = texture_resolve_variance[DTid.xy];
	bool strongBlur = variance > varianceEstimateThreshold;

	float radius = strongBlur ? bilateralMinMaxRadius.y : bilateralMinMaxRadius.x;
	radius = lerp(0.0, radius, saturate(roughness * 8.0)); // roughness 0.125 is destination

	float sigma = radius * BILATERAL_SIGMA;
	int effectiveRadius = min(sigma * 2.0, radius);

	if (variance > varianceExitThreshold && effectiveRadius > 0)
	{
		float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;
		float3 P = reconstruct_position(uv, depth);

		float4 result = 0;
		float weightSum = 0.0f;

		for (int r = -effectiveRadius; r <= effectiveRadius; r++)
		{
			const int2 sampleCoord = DTid.xy + (direction * r); // Left to right diameter directionally

			if (all(and(sampleCoord >= int2(0, 0), sampleCoord < (int2) postprocess.resolution)))
			{
				const float sampleDepth = texture_depth[sampleCoord];
				const float4 sampleColor = texture_temporal[sampleCoord];

				const float3 sampleN = decode_oct(texture_normal[sampleCoord]);
				const float sampleRoughness = texture_roughness[sampleCoord];

				float2 sampleUV = (sampleCoord + 0.5) * postprocess.resolution_rcp;
				float3 sampleP = reconstruct_position(sampleUV, sampleDepth);

				// Don't let invalid roughness samples interfere
				if (NeedReflection(sampleRoughness, sampleDepth, ssr_roughness_cutoff))
				{
					float3 dq = P - sampleP;
					float planeError = max(abs(dot(dq, sampleN)), abs(dot(dq, N)));
					float relativeDepthDifference = planeError / (linearDepth * GetCamera().z_far);
					float bilateralDepthWeight = exp(-sqr(relativeDepthDifference) * depthThreshold);

					float normalError = pow(saturate(dot(sampleN, N)), 4.0);
					float bilateralNormalWeight = saturate(1.0 - (1.0 - normalError) * normalThreshold);

					float bilateralWeight = bilateralDepthWeight * bilateralNormalWeight;

					float gaussian = exp(-sqr(r / sigma));
					float weight = (r == 0) ? 1.0 : gaussian * bilateralWeight; // Skip center gaussian peak

					result += sampleColor * weight;
					weightSum += weight;
				}
			}
		}

		result /= weightSum;
		outputColor = result;
	}

	output[DTid.xy] = outputColor;
}
