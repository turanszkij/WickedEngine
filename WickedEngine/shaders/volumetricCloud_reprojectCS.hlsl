#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> cloud_current : register(t0);
Texture2D<float2> cloud_depth_current : register(t1);
Texture2D<float4> cloud_history : register(t2);
Texture2D<float2> cloud_depth_history : register(t3);

RWTexture2D<float4> output : register(u0);
RWTexture2D<float2> output_depth : register(u1);

// This function compute the checkerboard undersampling position
int ComputeCheckerBoardIndex(int2 renderCoord, int subPixelIndex)
{
	const int localOffset = (renderCoord.x & 1 + renderCoord.y & 1) & 1;
	const int checkerBoardLocation = (subPixelIndex + localOffset) & 0x3;
	return checkerBoardLocation;
}

// Computes post-projection depth from linear depth
float getInverseLinearDepth(float lin, float near, float far)
{
	float z_n = ((lin - 2 * far) * near + far * lin) / (lin * near - far * lin);
	float z = (z_n + 1) / 2;
	return z;
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 renderCoord = DTid.xy / 2;
	const float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;
	
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
	
	float x = uv.x * 2 - 1;
	float y = (1 - uv.y) * 2 - 1;
	float2 screenPosition = float2(x, y);

	float currentCloudLinearDepth = cloud_depth_current.SampleLevel(sampler_point_clamp, uv, 0).x;
	float currentCloudDepth = getInverseLinearDepth(currentCloudLinearDepth, GetCamera().z_near, GetCamera().z_far);
	
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
	return;
#endif

	
	if (validHistory)
	{
		float4 newResult = cloud_current[renderCoord];
		float2 newDepthResult = cloud_depth_current[renderCoord];

		if (shouldUpdatePixel)
		{
			result = newResult;
			depthResult = newDepthResult;
		}
		else
		{
			float4 previousResult = cloud_history.SampleLevel(sampler_linear_clamp, prevUV, 0);
			float2 previousDepthResult = cloud_depth_history.SampleLevel(sampler_linear_clamp, prevUV, 0);

			result = previousResult;
			depthResult = previousDepthResult;
			
			float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 1).r; // Half res
			float3 depthWorldPosition = reconstruct_position(uv, depth);
			float tToDepthBuffer = length(depthWorldPosition - GetCamera().position);
			
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

						int2 neighborCoord = renderCoord + int2(x, y);
						
						float2 neighboorDepthResult = cloud_depth_current[neighborCoord];
						float neighborClosestDepth = abs(tToDepthBuffer - neighboorDepthResult.y);

						if (neighborClosestDepth < closestDepth)
						{
							closestDepth = neighborClosestDepth;
							float4 neighborResult = cloud_current[neighborCoord];

							result = neighborResult;
							depthResult = neighboorDepthResult;
						}
					}
				}

				if (abs(tToDepthBuffer - newDepthResult.y) < closestDepth)
				{
					result = newResult;
					depthResult = newDepthResult;
				}
			}
			else
			{
				
			}
		}
	}
	else
	{
		result = cloud_current.SampleLevel(sampler_linear_clamp, uv, 0);
		depthResult = cloud_depth_current.SampleLevel(sampler_linear_clamp, uv, 0);
	}

	output[DTid.xy] = result;
	output_depth[DTid.xy] = depthResult;
}
