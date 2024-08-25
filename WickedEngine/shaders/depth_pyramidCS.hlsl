#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(push, PostProcess);

Texture2D<float> input_depth : register(t0);
RWTexture2D<unorm float> output_mip0 : register(u0);
RWTexture2D<unorm float> output_mip1 : register(u1);

[numthreads(8, 8, 1)]
void main(uint2 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const uint2 GTid = remap_lane_8x8(groupIndex);
	const uint2 pixel = clamp(Gid.xy * 8 + GTid.xy, uint2(0, 0), push.resolution - 1);
	float2 uv = (pixel + 0.5) * push.resolution_rcp;
	float4 depths = input_depth.GatherRed(sampler_linear_clamp, uv, 0);
	float depth = min(depths.x, min(depths.y, min(depths.z, depths.w)));
	
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
