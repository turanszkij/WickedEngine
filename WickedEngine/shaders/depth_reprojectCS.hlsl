#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(push, PostProcess);

Texture2D<float> input_depth : register(t0);
Texture2D<float4> input_velocity : register(t1);

RWTexture2D<unorm float> output_mip0 : register(u0);
RWTexture2D<unorm float> output_mip1 : register(u1);
RWTexture2D<unorm float> output_mip2 : register(u2);
RWTexture2D<unorm float> output_mip3 : register(u3);

groupshared float shared_depths[2][2];

[numthreads(8, 8, 1)]
void main(uint2 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	ShaderCamera camera = GetCamera();
	const uint2 GTid = remap_lane_8x8(groupIndex);
	const uint2 pixel = Gid.xy * 8 + GTid.xy;
	float2 uv = (pixel + 0.5) * push.resolution_rcp;
	float2 velocity = input_velocity[pixel].xy;
	float2 uv_prev = uv + velocity;
	float depth = 0;

	if (is_saturated(uv_prev))
	{
		float4 depth_prev = input_depth.GatherRed(sampler_linear_clamp, uv_prev);
		depth = min(depth_prev.x, min(depth_prev.y, min(depth_prev.z, depth_prev.w)));

		float3 pos_prev = reconstruct_position(uv, depth, camera.previous_inverse_view_projection);
		float4 pos = mul(camera.view_projection, float4(pos_prev, 1));

		if (pos.w > 0)
		{
			pos.xyz /= pos.w;
			depth = pos.z;
		}
		else
		{
			depth = 0;
		}
	}

	output_mip0[pixel] = depth;

	float depth_x = QuadReadAcrossX(depth);
	float depth_y = QuadReadAcrossY(depth);
	float depth_d = QuadReadAcrossDiagonal(depth);
	depth = min(depth, min(depth_d, min(depth_x, depth_y)));
	if (all((GTid % 2) == 0))
	{
		output_mip1[pixel >> 1] = depth;
	}
	
	uint lane_index = WaveGetLaneIndex();
	depth_x = WaveReadLaneAt(depth, lane_index + 0x08);
	depth_y = WaveReadLaneAt(depth, lane_index + 0x04);
	depth_d = WaveReadLaneAt(depth, lane_index + 0x0c);
	depth = min(depth, min(depth_d, min(depth_x, depth_y)));
	if (all((GTid % 4) == 0))
	{
		output_mip2[pixel >> 2] = depth;
		shared_depths[GTid.x / 4][GTid.y / 4] = depth;
	}

	GroupMemoryBarrierWithGroupSync();

	if (all((GTid % 8) == 0))
	{
		depth = min(shared_depths[0][0], min(shared_depths[0][1], min(shared_depths[1][0], shared_depths[1][1])));
		output_mip3[pixel >> 3] = depth;
	}
}
