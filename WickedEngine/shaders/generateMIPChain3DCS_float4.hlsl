#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

#ifndef MIP_OUTPUT_FORMAT
#define MIP_OUTPUT_FORMAT float4
#endif

PUSHCONSTANT(push, GenerateMIPChainCB);

TEXTURE3D(input, float4, TEXSLOT_ONDEMAND0);
RWTEXTURE3D(output, MIP_OUTPUT_FORMAT, 0);

SAMPLERSTATE(customsampler, SSLOT_ONDEMAND0);

[numthreads(GENERATEMIPCHAIN_3D_BLOCK_SIZE, GENERATEMIPCHAIN_3D_BLOCK_SIZE, GENERATEMIPCHAIN_3D_BLOCK_SIZE)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	if (DTid.x < push.outputResolution.x && DTid.y < push.outputResolution.y && DTid.z < push.outputResolution.z)
	{
		output[DTid] = input.SampleLevel(customsampler, ((float3)DTid + 0.5f) * (float3)push.outputResolution_rcp, 0);
	}
}
