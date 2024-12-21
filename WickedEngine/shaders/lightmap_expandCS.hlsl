#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

Texture2D lightmap_input : register(t0);

RWTexture2D<float4> lightmap_output : register(u0);

static const int2 offsets[] = {
	int2(0, -1),
	int2(0, 1),
	int2(-1, 0),
	int2(1, 0),
	
	int2(-1, -1),
	int2(1, -1),
	int2(1, 1),
	int2(-1, -1),
};

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int2 pixel = DTid.xy;
	float4 color = lightmap_input[pixel];

	for (uint i = 0; (i < arraysize(offsets)) && (color.a < 1); ++i)
	{
		color = lightmap_input[pixel + offsets[i]];
	}
	
	lightmap_output[pixel] = color;
}
