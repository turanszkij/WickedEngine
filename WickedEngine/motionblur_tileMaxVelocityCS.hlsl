#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

RWTEXTURE2D(output, float2, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 tile_upperleft = DTid.xy * MOTIONBLUR_TILESIZE;
	float max_magnitude = 0;
	float2 max_velocity = 0;

	[loop]
	for (uint t = 0; t < MOTIONBLUR_TILESIZE * MOTIONBLUR_TILESIZE; ++t)
	{
		const uint2 pixel = tile_upperleft + unflatten2D(t, MOTIONBLUR_TILESIZE);
		const float2 velocity = texture_gbuffer1[pixel].zw;
		const float magnitude = length(velocity);
		if (magnitude > max_magnitude)
		{
			max_magnitude = magnitude;
			max_velocity = velocity;
		}
	}

	output[DTid.xy] = max_velocity;
}