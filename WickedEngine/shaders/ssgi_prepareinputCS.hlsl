#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D input : register(t0);

RWTexture2D<float4> output : register(u0);

groupshared float3 shared_colors[2][2];

[numthreads(8, 8, 1)]
void main(uint2 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const uint2 GTid = remap_lane_8x8(groupIndex);
	const uint2 pixel = Gid.xy * POSTPROCESS_BLOCKSIZE + GTid.xy;
	const float2 uv = (pixel + 0.5) * postprocess.resolution_rcp;
	const float2 velocity = texture_velocity.SampleLevel(sampler_linear_clamp, uv, 0);
	const float2 prevUV = uv + velocity;
	
	float3 rrr = input.GatherRed(sampler_linear_clamp, prevUV).rgb;
	float3 ggg = input.GatherGreen(sampler_linear_clamp, prevUV).rgb;
	float3 bbb = input.GatherBlue(sampler_linear_clamp, prevUV).rgb;
	
#if 1
	// Disocclusion fallback:
	float depth_current = texture_lineardepth.SampleLevel(sampler_point_clamp, uv, 0) * GetCamera().z_far;
	float depth_history = compute_lineardepth(texture_depth_history.SampleLevel(sampler_point_clamp, prevUV, 0));
	if (length(velocity) > 0.01 && abs(depth_current - depth_history) > 1)
	{
		rrr = 0;
		ggg = 0;
		bbb = 0;
	}
#endif

	float r = max3(rrr);
	float g = max3(ggg);
	float b = max3(bbb);

	float3 color = float3(r, g, b);
	
	// add some energy loss because texture is accumulative:
	color -= 0.1;
	color *= 0.8;
	
	color = clamp(color, 0, 65000);

	// Reduce with MAX filter:

	color = max(color, QuadReadAcrossX(color));
	color = max(color, QuadReadAcrossY(color));
	color = max(color, QuadReadAcrossDiagonal(color));
	
	uint lane_index = WaveGetLaneIndex();
	if(all(GTid % 4 == 0))
	{
		color = max(color, WaveReadLaneAt(color, lane_index + 0x08));
		color = max(color, WaveReadLaneAt(color, lane_index + 0x04));
		color = max(color, WaveReadLaneAt(color, lane_index + 0x0c));
		shared_colors[GTid.x / 4][GTid.y / 4] = color;
	}
	
	GroupMemoryBarrierWithGroupSync();

	if(groupIndex == 0)
	{
		float3 finalColor = 0;
#if 1
		finalColor = max(finalColor, shared_colors[0][0]);
		finalColor = max(finalColor, shared_colors[1][0]);
		finalColor = max(finalColor, shared_colors[0][1]);
		finalColor = max(finalColor, shared_colors[1][1]);
#else
		finalColor += shared_colors[0][0];
		finalColor += shared_colors[1][0];
		finalColor += shared_colors[0][1];
		finalColor += shared_colors[1][1];
		finalColor /= 4.0;
#endif
		output[Gid.xy] = float4(finalColor, 1);
	}
}
