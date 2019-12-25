#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(tilemax_horizontal, float2, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, float2, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 tile_upperleft = uint2(DTid.x, DTid.y * MOTIONBLUR_TILESIZE.y);
	float max_magnitude = 0;
	float2 max_velocity = 0;

	[loop]
	for (uint i = 0; i < MOTIONBLUR_TILESIZE.y; ++i)
	{
		const uint2 pixel = uint2(tile_upperleft.x, tile_upperleft.y + i);
		const float2 velocity = tilemax_horizontal[pixel];
		const float magnitude = length(velocity);
		if (magnitude > max_magnitude)
		{
			max_magnitude = magnitude;
			max_velocity = velocity;
		}
	}

	output[DTid.xy] = max_velocity;
}