#include "globals.hlsli"
#include "generateMIPChainHF.hlsli"

TEXTURE3D(input, float4, TEXSLOT_UNIQUE0);
RWTEXTURE3D(output, float4, 0);

SAMPLERSTATE(customsampler, SSLOT_ONDEMAND0);

[numthreads(GENERATEMIPCHAIN_3D_BLOCK_SIZE, GENERATEMIPCHAIN_3D_BLOCK_SIZE, GENERATEMIPCHAIN_3D_BLOCK_SIZE)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint3 dim;
	output.GetDimensions(dim.x, dim.y, dim.z);

	if (DTid.x < dim.x && DTid.y < dim.y && DTid.z < dim.z)
	{
		output[DTid] = input.SampleLevel(customsampler, ((float3)DTid + 0.5f) / (float3)dim, 0);
	}
}