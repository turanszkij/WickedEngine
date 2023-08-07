#include "globals.hlsli"
#include "volumetricCloudsHF.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> cloud_current : register(t0);
Texture2D<float2> cloud_depth_current : register(t1);
Texture2D<float4> cloud_history : register(t2);
Texture2D<float2> cloud_depth_history : register(t3);
Texture2D<float> cloud_additional_history : register(t4);

RWTexture2D<float4> output : register(u0);
RWTexture2D<float2> output_depth : register(u1);
RWTexture2D<float> output_additional : register(u2);
RWTexture2D<unorm float4> output_cloudMask : register(u3);

static const float2 temporalResponseMinMax = float2(0.5, 0.9);

// When moving fast reprojection cannot catch up. This value eliminates the ghosting but results in clipping artefacts
//#define ADDITIONAL_BOX_CLAMP

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;
	
	uint2 renderResolution = postprocess.resolution / 2;
	uint2 renderCoord = DTid.xy / 2;

	uint2 minRenderCoord = uint2(0, 0);
	uint2 maxRenderCoord = renderResolution - 1;
	
#if 0
	
    // Calculate screen dependant motion vector
    float4 prevPos = float4(uv * 2.0 - 1.0, 1.0, 1.0);
    prevPos = mul(GetCamera().inverse_projection, prevPos);
    prevPos = prevPos / prevPos.w;
	
    prevPos.xyz = mul((float3x3)GetCamera().inverse_view, prevPos.xyz);
    prevPos.xyz = mul((float3x3)GetCamera().previous_view, prevPos.xyz);
	
    float4 reproj = mul(GetCamera().projection, prevPos);
    reproj /= reproj.w;
	
    float2 prevUV = reproj.xy * 0.5 + 0.5;
	
#else
	
	float2 screenPosition = uv_to_clipspace(uv);

	float currentCloudLinearDepth = cloud_depth_current.SampleLevel(sampler_point_clamp, uv, 0).x;
	float currentCloudDepth = compute_inverse_lineardepth(currentCloudLinearDepth, GetCamera().z_near, GetCamera().z_far);
	
	float4 thisClip = float4(screenPosition, currentCloudDepth, 1.0);
	
	float4 prevClip = mul(GetCamera().inverse_view_projection, thisClip);
	prevClip = mul(GetCamera().previous_view_projection, prevClip);
	
	//float4 prevClip = mul(GetCamera().previous_view_projection, worldPosition);
	float2 prevScreen = prevClip.xy / prevClip.w;
	
	float2 screenVelocity = screenPosition - prevScreen;
	float2 prevScreenPosition = screenPosition - screenVelocity;
	
	// Transform from screen position to uv
	float2 prevUV = prevScreenPosition * float2(0.5, -0.5) + 0.5;
    
#endif
	
	bool validHistory = is_saturated(prevUV) && volumetricclouds_frame > 0;

	int subPixelIndex = GetFrame().frame_count % 4;
	int localIndex = (DTid.x & 1) + (DTid.y & 1) * 2;
	int currentIndex = ComputeCheckerBoardIndex(renderCoord, subPixelIndex);
	
	bool shouldUpdatePixel = (localIndex == currentIndex);
	
	float4 result = 0.0;
	float2 depthResult = 0.0;
	float additionalResult = 0.0;


#if 0 // Simple reprojection version
	if (shouldUpdatePixel)
	{
		result = cloud_current[renderCoord];
		depthResult = cloud_depth_current[renderCoord];
	}
	else
	{
		result = cloud_history.SampleLevel(sampler_linear_clamp, uv, 0);
		depthResult = cloud_depth_history.SampleLevel(sampler_linear_clamp, uv, 0);
	}
	output[DTid.xy] = result;
	output_depth[DTid.xy] = depthResult;
	output_additional[DTid.xy] = 0.0;
	output_cloudMask[DTid.xy] = pow(saturate(1 - result.a), 64);
	return;
#endif

	
	if (validHistory)
	{
		float4 newResult = cloud_current[clamp(renderCoord, minRenderCoord, maxRenderCoord)];
		float2 newDepthResult = cloud_depth_current[clamp(renderCoord, minRenderCoord, maxRenderCoord)];

		float4 previousResult = cloud_history.SampleLevel(sampler_linear_clamp, prevUV, 0);
		float2 previousDepthResult = cloud_depth_history.SampleLevel(sampler_linear_clamp, prevUV, 0);
		float previousAdditionalResult = cloud_additional_history.SampleLevel(sampler_linear_clamp, prevUV, 0);
		
		float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 1).r; // Half res
		float3 depthWorldPosition = reconstruct_position(uv, depth);
		float tToDepthBuffer = length(depthWorldPosition - GetCamera().position);

		// Fow now we use float values instead of half-float, we need to convert the cloud system to use kilometer unit and we can revert to half-float values
		tToDepthBuffer = depth == 0.0 ? FLT_MAX : tToDepthBuffer;

		if (shouldUpdatePixel)
		{
			if (abs(tToDepthBuffer - previousDepthResult.y) > tToDepthBuffer * 0.1)
			{
				result = newResult;
				depthResult = newDepthResult;
				additionalResult = temporalResponseMinMax.x;
			}
			else
			{
				// Based on Welford's online algorithm:
				//  https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
				
				float4 m1 = 0.0;
				float4 m2 = 0.0;
				for (int x = -1; x <= 1; x++)
				{
					for (int y = -1; y <= 1; y++)
					{
						int2 offset = int2(x, y);
						int2 neighborCoord = renderCoord + offset;
						neighborCoord = clamp(neighborCoord, (int2)minRenderCoord, (int2)maxRenderCoord);
						
						float4 neighborResult = cloud_current[neighborCoord];

						m1 += neighborResult;
						m2 += neighborResult * neighborResult;
					}
				}

				float4 mean = m1 / 9.0;
				float4 variance = (m2 / 9.0) - (mean * mean);
				float4 stddev = sqrt(max(variance, 0.0f));

				// Color box clamp
				float4 colorMin = mean - stddev;
				float4 colorMax = mean + stddev;
				previousResult = clamp(previousResult, colorMin, colorMax);
				
				result = lerp(newResult, previousResult, previousAdditionalResult);
				depthResult = newDepthResult;
				additionalResult = temporalResponseMinMax.y;
			}
		}
		else
		{
			result = previousResult;
			depthResult = previousDepthResult;
			additionalResult = previousAdditionalResult;
			
			if (abs(tToDepthBuffer - previousDepthResult.y) > tToDepthBuffer * 0.1)
			{
				float closestDepth = FLT_MAX;
				for (int y = -1; y <= 1; y++)
				{
					for (int x = -1; x <= 1; x++)
					{
						// If it's middle then skip. We only evaluate neighbor samples
						if ((abs(x) + abs(y)) == 0)
							continue;

						int2 offset = int2(x, y);
						int2 neighborCoord = renderCoord + offset;
						neighborCoord = clamp(neighborCoord, (int2)minRenderCoord, (int2)maxRenderCoord);

						float2 neighboorDepthResult = cloud_depth_current[neighborCoord];
						float neighborClosestDepth = abs(tToDepthBuffer - neighboorDepthResult.y);

						if (neighborClosestDepth < closestDepth)
						{
							closestDepth = neighborClosestDepth;
							
							float4 neighborResult = cloud_current[neighborCoord];
							
							result = neighborResult;
							depthResult = neighboorDepthResult;
							additionalResult = temporalResponseMinMax.x;
						}
					}
				}
			}
			else
			{
#ifdef ADDITIONAL_BOX_CLAMP
				// Simple box clamping from neighbour pixels
				float4 resultAABBMin = FLT_MAX;
				float4 resultAABBMax = 0.0;
				float2 depthResultAABBMin = FLT_MAX;
				float2 depthResultAABBMax = 0.0;

				for (int y = -1; y <= 1; y++)
				{
					for (int x = -1; x <= 1; x++)
					{
					// If it's middle then skip. We only evaluate neighbor samples
						if ((abs(x) + abs(y)) == 0)
							continue;

						int2 offset = int2(x, y);
						int2 neighborCoord = renderCoord + offset;
							
						float4 neighborResult = cloud_current[neighborCoord];
						float2 neighboorDepthResult = cloud_depth_current[neighborCoord];

						resultAABBMin = min(resultAABBMin, neighborResult);
						resultAABBMax = max(resultAABBMax, neighborResult);
						depthResultAABBMin = min(depthResultAABBMin, neighboorDepthResult);
						depthResultAABBMax = max(depthResultAABBMax, neighboorDepthResult);
					}
				}

				resultAABBMin = min(resultAABBMin, newResult);
				resultAABBMax = max(resultAABBMax, newResult);
				depthResultAABBMin = min(depthResultAABBMin, newDepthResult);
				depthResultAABBMax = max(depthResultAABBMax, newDepthResult);

				result = clamp(result, resultAABBMin, resultAABBMax);
				depthResult = clamp(depthResult, depthResultAABBMin, depthResultAABBMax);
#endif // ADDITIONAL_CLIPPING
			}
		}
	}
	else
	{
		result = cloud_current.SampleLevel(sampler_linear_clamp, uv, 0);
		depthResult = cloud_depth_current.SampleLevel(sampler_linear_clamp, uv, 0);
		additionalResult = temporalResponseMinMax.x;
	}
	
	output[DTid.xy] = result;
	output_depth[DTid.xy] = depthResult;
	output_additional[DTid.xy] = additionalResult;
	output_cloudMask[DTid.xy] = pow(saturate(1 - result.a), 64);
}
