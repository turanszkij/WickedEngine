#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> aerialeperspective_current : register(t0);
Texture2D<float4> aerialeperspective_history : register(t1);

RWTexture2D<float4> output : register(u0);

static const float temporalResponse = 0.9;
static const float temporalScale = 1.0;

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;
	float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 1).r;

	// Trim
	if (depth == 0.0)
	{
		output[DTid.xy] = float4(0.0, 0.0, 0.0, 0.0);
		return;
	}
	
	uint2 renderResolution = postprocess.resolution;
	uint2 renderCoord = DTid.xy;

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
	float4 thisClip = float4(screenPosition, depth, 1.0);
	
	float4 prevClip = mul(GetCamera().inverse_view_projection, thisClip);
	prevClip = mul(GetCamera().previous_view_projection, prevClip);
	
	float2 prevScreen = prevClip.xy / prevClip.w;
	
	float2 screenVelocity = screenPosition - prevScreen;
	float2 prevScreenPosition = screenPosition - screenVelocity;
	
	// Transform from screen position to uv
	float2 prevUV = clipspace_to_uv(prevScreenPosition);
    
#endif
	
	// Based on Welford's online algorithm:
	//  https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
	
	float4 m1 = 0.0;
	float4 m2 = 0.0;
	float validCount = 0.0;
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			int2 offset = int2(x, y);
			
			int2 neighborTracingCoord = renderCoord + offset;
			int2 neighborCoord = DTid.xy + offset;
			
			float2 neighborUV = neighborCoord * postprocess.resolution_rcp;
			
			float4 neighborResult = aerialeperspective_current[neighborTracingCoord];
			float neighborDepth = texture_depth.SampleLevel(sampler_point_clamp, neighborUV, 1);

			if (neighborDepth > 0.0)
			{
				m1 += neighborResult;
				m2 += neighborResult * neighborResult;
				validCount += 1.0;
			}
		}
	}

	float4 mean = m1 / validCount;
	float4 variance = (m2 / validCount) - (mean * mean);
	float4 stddev = sqrt(max(variance, 0.0f));

	// Color box clamp
	float4 colorMin = mean - temporalScale * stddev;
	float4 colorMax = mean + temporalScale * stddev;

	float4 previousResult = aerialeperspective_history.SampleLevel(sampler_linear_clamp, prevUV, 0);
	previousResult = clamp(previousResult, colorMin, colorMax);
	
	float4 newResult = aerialeperspective_current[clamp(renderCoord, minRenderCoord, maxRenderCoord)];	
	float4 result = lerp(newResult, previousResult, temporalResponse);
	
	output[DTid.xy] = result;
}
