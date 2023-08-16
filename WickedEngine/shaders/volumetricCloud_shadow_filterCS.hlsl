#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float3> input : register(t0);
RWTexture2D<float3> output : register(u0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5) * postprocess.resolution_rcp;

#if 0 // Debug
	output[DTid.xy] = input[DTid.xy];
	return;
#endif
	
	// Filter
	float3 filteredResult = 0.0;
	float sum = 0.0;

	const int radius = 1;
	for (int x = -radius; x <= radius; x++)
	{
		for (int y = -radius; y <= radius; y++)
		{
			int2 offset = int2(x, y);
			int2 neighborCoord = DTid.xy + offset;

			if (all(and(neighborCoord >= int2(0, 0), neighborCoord < (int2) postprocess.resolution)))
			{
				float3 neighborResult = input[neighborCoord];
				
				filteredResult += neighborResult;
				sum += 1.0;
			}
		}
	}
	filteredResult /= sum;

	output[DTid.xy] = filteredResult;
}
