#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(tilemax_horizontal, float2, TEXSLOT_ONDEMAND0);
TEXTURE2D(tilemin_horizontal, float2, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(tilemax, float2, 0);
RWTEXTURE2D(tilemin, float2, 1);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 tile_upperleft = uint2(DTid.x, DTid.y * MOTIONBLUR_TILESIZE);
	float max_magnitude = 0;
	float2 max_velocity = 0;
	float min_magnitude = 100000;
	float2 min_velocity = 100000;

	[loop]
	for (uint i = 0; i < MOTIONBLUR_TILESIZE; ++i)
	{
		const uint2 pixel = uint2(tile_upperleft.x, tile_upperleft.y + i);
		float2 velocity = tilemax_horizontal[pixel];
		float magnitude = length(velocity);
		if (magnitude > max_magnitude)
		{
			max_magnitude = magnitude;
			max_velocity = velocity;
		}

		velocity = tilemin_horizontal[pixel];
		magnitude = length(velocity);
		if (magnitude < min_magnitude)
		{
			min_magnitude = magnitude;
			min_velocity = velocity;
		}
	}

	tilemax[DTid.xy] = max_velocity;
	tilemin[DTid.xy] = min_velocity;
}