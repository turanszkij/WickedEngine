#include "globals.hlsli"
#include "ShaderInterop_Utility.h"

#ifndef MIP_OUTPUT_FORMAT
#define MIP_OUTPUT_FORMAT float4
#endif

TEXTURE3D(input, float4, TEXSLOT_UNIQUE0);
RWTEXTURE3D(output, MIP_OUTPUT_FORMAT, 0);

SAMPLERSTATE(customsampler, SSLOT_ONDEMAND0);

[numthreads(GENERATEMIPCHAIN_3D_BLOCK_SIZE, GENERATEMIPCHAIN_3D_BLOCK_SIZE, GENERATEMIPCHAIN_3D_BLOCK_SIZE)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	if (DTid.x < outputResolution.x && DTid.y < outputResolution.y && DTid.z < outputResolution.z)
	{
		output[DTid] = input.SampleLevel(customsampler, ((float3)DTid + 0.5f) * (float3)outputResolution_rcp, 0);
	}
}