#include "globals.hlsli"
#include "ShaderInterop_Utility.h"

#ifndef MIP_OUTPUT_FORMAT
#define MIP_OUTPUT_FORMAT float4
#endif

TEXTURECUBE(input, float4, TEXSLOT_UNIQUE0);
RWTEXTURE2DARRAY(output, MIP_OUTPUT_FORMAT, 0);

SAMPLERSTATE(customsampler, SSLOT_ONDEMAND0);

[numthreads(GENERATEMIPCHAIN_2D_BLOCK_SIZE, GENERATEMIPCHAIN_2D_BLOCK_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x < outputResolution.x && DTid.y < outputResolution.y)
	{
		float2 uv = ((float2)DTid.xy + 0.5f) * outputResolution_rcp.xy;
		float3 N = UV_to_CubeMap(uv, DTid.z);

		output[DTid.xyz] = input.SampleLevel(customsampler, N, 0);
	}
}
