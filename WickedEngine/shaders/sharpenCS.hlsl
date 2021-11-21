#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> input : register(t0);

RWTexture2D<float4> output : register(u0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float4 center = input[DTid.xy + int2(0, 0)];
	float4 top =	input[DTid.xy + int2(0, -1)];
	float4 left =	input[DTid.xy + int2(-1, 0)];
	float4 right =	input[DTid.xy + int2(1, 0)];
	float4 bottom = input[DTid.xy + int2(0, 1)];

	output[DTid.xy] = center + (4 * center - top - bottom - left - right) * postprocess.params0[0];
}
