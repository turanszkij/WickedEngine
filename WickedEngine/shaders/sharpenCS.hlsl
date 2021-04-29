#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, unorm float4, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float4 center = input[DTid.xy + int2(0, 0)];
	float4 top =	input[DTid.xy + int2(0, -1)];
	float4 left =	input[DTid.xy + int2(-1, 0)];
	float4 right =	input[DTid.xy + int2(1, 0)];
	float4 bottom = input[DTid.xy + int2(0, 1)];

	output[DTid.xy] = saturate(center + (4 * center - top - bottom - left - right) * xPPParams0[0]);
}