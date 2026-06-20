#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(push, PostProcess);
Texture2D<float> input_depth : register(t0);

RWTexture2D<unorm float> output_mip0 : register(u0);
RWTexture2D<unorm float> output_mip1 : register(u1);
RWTexture2D<unorm float> output_mip2 : register(u2);
RWTexture2D<unorm float> output_mip3 : register(u3);

groupshared float shared_depths[8][8];

[numthreads(64, 1, 1)]
void main(uint2 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const uint2 GTid = remap_lane_8x8(groupIndex);
	const uint2 pixel = clamp(Gid.xy * 8 + GTid.xy, uint2(0, 0), push.resolution - 1);
	float2 uv = (pixel + 0.5) * push.resolution_rcp;
	
	float4 depths = input_depth.GatherRed(sampler_linear_clamp, uv, 0);
	float depth = min(depths.x, min(depths.y, min(depths.z, depths.w)));
	output_mip0[pixel] = depth;

	float min_x = min(depth, QuadReadAcrossX(depth));
	depth = min(min_x, QuadReadAcrossY(min_x));
	
	shared_depths[GTid.y][GTid.x] = depth;
	
	if ((GTid.x % 2 == 0) && (GTid.y % 2 == 0))
	{
		output_mip1[pixel >> 1] = depth;
	}
	GroupMemoryBarrierWithGroupSync();

	if ((GTid.x % 4 == 0) && (GTid.y % 4 == 0))
	{
		float d0 = shared_depths[GTid.y    ][GTid.x    ];
		float d1 = shared_depths[GTid.y    ][GTid.x + 2];
		float d2 = shared_depths[GTid.y + 2][GTid.x    ];
		float d3 = shared_depths[GTid.y + 2][GTid.x + 2];
		
		depth = min(min(d0, d1), min(d2, d3));
		output_mip2[pixel >> 2] = depth;
		
		shared_depths[GTid.y][GTid.x] = depth;
	}

	GroupMemoryBarrierWithGroupSync();

	if (groupIndex == 0)
	{
		float d0 = shared_depths[0][0];
		float d1 = shared_depths[0][4];
		float d2 = shared_depths[4][0];
		float d3 = shared_depths[4][4];
		
		depth = min(min(d0, d1), min(d2, d3));
		output_mip3[pixel >> 3] = depth;
	}
}
