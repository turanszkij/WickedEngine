#include "globals.hlsli"
#include "generateMIPChainHF.hlsli"

#ifndef MIP_OUTPUT_FORMAT
#define MIP_OUTPUT_FORMAT float4
#endif

TEXTURE2D(input, float4, TEXSLOT_UNIQUE0);
RWTEXTURE2D(output, MIP_OUTPUT_FORMAT, 0);

SAMPLERSTATE(customsampler, SSLOT_ONDEMAND0);

[numthreads(GENERATEMIPCHAIN_2D_BLOCK_SIZE, GENERATEMIPCHAIN_2D_BLOCK_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x < outputResolution.x && DTid.y < outputResolution.y)
	{
		output[DTid.xy] = input.SampleLevel(customsampler, (DTid.xy + 1.0f) / (float2)outputResolution.xy, 0);
	}
}