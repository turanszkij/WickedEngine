#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

#ifndef MIP_OUTPUT_FORMAT
#define MIP_OUTPUT_FORMAT float4
#endif

PUSHCONSTANT(mipgen, MipgenPushConstants);

[numthreads(GENERATEMIPCHAIN_2D_BLOCK_SIZE, GENERATEMIPCHAIN_2D_BLOCK_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x < mipgen.outputResolution.x && DTid.y < mipgen.outputResolution.y)
	{
		SamplerState customsampler = bindless_samplers[mipgen.sampler_index];
		TextureCubeArray input = bindless_cubearrays[mipgen.texture_input];
		RWTexture2DArray<MIP_OUTPUT_FORMAT> output = bindless_rwtextures2DArray[mipgen.texture_output];

		float2 uv = ((float2)DTid.xy + 0.5f) * mipgen.outputResolution_rcp.xy;
		float3 N = UV_to_CubeMap(uv, DTid.z);

		output[uint3(DTid.xy, DTid.z + mipgen.arrayIndex * 6)] = input.SampleLevel(customsampler, float4(N, mipgen.arrayIndex), 0);
	}
}
