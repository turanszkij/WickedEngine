#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float2, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, float2, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float max_magnitude = 0;
	float2 max_velocity = 0;

	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			const float2 velocity = input[DTid.xy + int2(x, y)];
			const float magnitude = length(velocity);
			if (magnitude > max_magnitude)
			{
				max_magnitude = magnitude;
				max_velocity = velocity;
			}
		}
	}

	output[DTid.xy] = max_velocity;
}