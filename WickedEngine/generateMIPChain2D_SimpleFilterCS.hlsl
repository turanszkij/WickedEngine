#include "globals.hlsli"
#include "generateMIPChainHF.hlsli"

TEXTURE2D(input, float4, TEXSLOT_UNIQUE0);
RWTEXTURE2D(output, float4, 0);

SAMPLERSTATE(customsampler, SSLOT_ONDEMAND0);

[numthreads(GENERATEMIPCHAIN_2D_BLOCK_SIZE, GENERATEMIPCHAIN_2D_BLOCK_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 dim;
	output.GetDimensions(dim.x, dim.y);

	if (DTid.x < dim.x && DTid.y < dim.y)
	{
		output[DTid.xy] = input.SampleLevel(customsampler, ((float2)DTid + 0.5f) / (float2)dim, 0);
	}
}