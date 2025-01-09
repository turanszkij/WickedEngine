#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

#ifndef MIP_OUTPUT_FORMAT
#define MIP_OUTPUT_FORMAT float4
#endif

PUSHCONSTANT(mipgen, MipgenPushConstants);

[numthreads(GENERATEMIPCHAIN_3D_BLOCK_SIZE, GENERATEMIPCHAIN_3D_BLOCK_SIZE, GENERATEMIPCHAIN_3D_BLOCK_SIZE)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x < mipgen.outputResolution.x && DTid.y < mipgen.outputResolution.y && DTid.z < mipgen.outputResolution.z)
	{
		SamplerState customsampler = bindless_samplers[descriptor_index(mipgen.sampler_index)];
		Texture3D input = bindless_textures3D[descriptor_index(mipgen.texture_input)];
		RWTexture3D<MIP_OUTPUT_FORMAT> output = bindless_rwtextures3D[descriptor_index(mipgen.texture_output)];

		output[DTid] = input.SampleLevel(customsampler, ((float3)DTid + 0.5f) * (float3)mipgen.outputResolution_rcp, 0);
	}
}
