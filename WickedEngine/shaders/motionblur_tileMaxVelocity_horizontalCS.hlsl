#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

RWTexture2D<float2> tilemax_horizontal : register(u0);
RWTexture2D<float2> tilemin_horizontal : register(u1);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 tile_upperleft = uint2(DTid.x * MOTIONBLUR_TILESIZE.x, DTid.y);
	float max_magnitude = 0;
	float2 max_velocity = 0;
	float min_magnitude = 100000;
	float2 min_velocity = 100000;

	[loop]
	for (uint i = 0; i < MOTIONBLUR_TILESIZE.x; ++i)
	{
		const uint2 pixel = uint2(tile_upperleft.x + i, tile_upperleft.y);
		const float2 uv = (pixel + 0.5) * postprocess.resolution_rcp;
		const float2 velocity = texture_velocity.SampleLevel(sampler_point_clamp, uv, 0).xy;
		const float magnitude = length(velocity);
		if (magnitude > max_magnitude)
		{
			max_magnitude = magnitude;
			max_velocity = velocity;
		}
		if (magnitude < min_magnitude)
		{
			min_magnitude = magnitude;
			min_velocity = velocity;
		}
	}

	tilemax_horizontal[DTid.xy] = max_velocity;
	tilemin_horizontal[DTid.xy] = min_velocity;
}
