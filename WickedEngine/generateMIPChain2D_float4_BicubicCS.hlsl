#include "globals.hlsli"
#include "ShaderInterop_Utility.h"

#ifndef MIP_OUTPUT_FORMAT
#define MIP_OUTPUT_FORMAT float4
#endif

TEXTURE2D(input, float4, TEXSLOT_UNIQUE0);
RWTEXTURE2D(output, MIP_OUTPUT_FORMAT, 0);

[numthreads(GENERATEMIPCHAIN_2D_BLOCK_SIZE, GENERATEMIPCHAIN_2D_BLOCK_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x < outputResolution.x && DTid.y < outputResolution.y)
	{
		output[DTid.xy] = SampleTextureCatmullRom(input, ((float2)DTid.xy + 0.5f) / (float2)outputResolution.xy);
	}
}