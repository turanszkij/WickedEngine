#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<uint> input : register(t0);

RWTexture2D<float4> output : register(u0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
	const uint primitiveID = input[DTid];

	if (primitiveID != 0)
	{
		uint meshletIndex = primitiveID & (~0u >> 7u);
		meshletIndex -= 1; // remove valid check
		ShaderMeshlet meshlet = load_meshlet(meshletIndex);
		output[DTid.xy] = float4(random_color(meshlet.instanceIndex + meshlet.primitiveOffset), 1);
	}
}
