#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(push, PostProcess);

Texture2D<float> input_depth : register(t0);
Texture2D<float4> input_velocity : register(t1);

RWTexture2D<unorm float> output_mip0 : register(u0);
RWTexture2D<unorm float> output_mip1 : register(u1);

[numthreads(8, 8, 1)]
void main(uint2 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	ShaderCamera camera = GetCamera();
	const uint2 GTid = remap_lane_8x8(groupIndex);
	const uint2 pixel = clamp(Gid.xy * 8 + GTid.xy, uint2(0, 0), push.resolution - 1);
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
}
